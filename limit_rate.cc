/** @file
 * limit-rate.cc
 *
 *  Created on: 2016年9月1日
 *      Author: xie

 Plugin to perform background fetches of certain content that would
 otherwise not be cached. For example, Range: requests / responses.

 @section license License

 Licensed to the Apache Software Foundation (ASF) under one
 or more contributor license agreements.  See the NOTICE file
 distributed with this work for additional information
 regarding copyright ownership.  The ASF licenses this file
 to you under the Apache License, Version 2.0 (the
 "License"); you may not use this file except in compliance
 with the License.  You may obtain a copy of the License at

 http://www.apache.org/licenses/LICENSE-2.0

 Unless required by applicable law or agreed to in writing, software
 distributed under the License is distributed on an "AS IS" BASIS,
 WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 See the License for the specific language governing permissions and
 limitations under the License.
 */

#include "common.h"
#include "configuration.h"
#include "ratelimiter.h"

#include <ts/ts.h>
#include <ts/ink_inet.h>
#include <ts/experimental.h>
#include <ts/remap.h>

typedef struct {
	TSVIO output_vio;
	TSIOBuffer output_buffer;
	TSIOBufferReader output_reader;
	TSHttpTxn txnp;bool ready;
	LimiterState *state;
	RateLimiter *rate_limiter;
} TransformData;

static TransformData *
my_data_alloc() {
	TransformData *data;

	data = (TransformData *) TSmalloc(sizeof(TransformData));
	data->output_vio = NULL;
	data->output_buffer = NULL;
	data->output_reader = NULL;
	data->txnp = NULL;
	data->state = NULL;
	data->rate_limiter = NULL;
	data->ready = false;
	return data;
}

static void my_data_destroy(TransformData * data) {
	TSReleaseAssert(data);
	if (data) {
		if (data->output_buffer) {
			TSIOBufferReaderFree(data->output_reader);
			TSIOBufferDestroy(data->output_buffer);
		}
		if (data->state) {
			delete data->state;
			data->state = NULL;
		}
		if (data->rate_limiter) {
//			data->rate_limiter->release();
			data->rate_limiter = NULL;
		}
		TSDebug(PLUGIN_NAME, "my_data_destroy");
		TSfree(data);
		data = NULL;
	}
}

static void handle_rate_limiting_transform(TSCont contp) {
	TSVConn output_conn;
	TSVIO input_vio;
	TransformData *data;
	int64_t towrite;
	int64_t avail;

	output_conn = TSTransformOutputVConnGet(contp);
	input_vio = TSVConnWriteVIOGet(contp);

	data = (TransformData *) TSContDataGet(contp);
	if (!data->ready) {
		data->output_buffer = TSIOBufferCreate();
		data->output_reader = TSIOBufferReaderAlloc(data->output_buffer);
		data->output_vio = TSVConnWrite(output_conn, contp, data->output_reader,
				TSVIONBytesGet(input_vio));
		data->ready = true;
	}

	if (!TSVIOBufferGet(input_vio)) {
		TSVIONBytesSet(data->output_vio, TSVIONDoneGet(input_vio));
		TSVIOReenable(data->output_vio);
		return;
	}

	towrite = TSVIONTodoGet(input_vio);

	if (towrite > 0) {
		avail = TSIOBufferReaderAvail(TSVIOReaderGet(input_vio));

		if (towrite > avail)
			towrite = avail;

		if (towrite > 0) {

			int64_t rl_max = data->rate_limiter->getMaxUnits(towrite,
					data->state);
			//if (rl_max <= towrite)
			towrite = rl_max;

			if (towrite) {
				TSIOBufferCopy(TSVIOBufferGet(data->output_vio),
						TSVIOReaderGet(input_vio), towrite, 0);
				TSIOBufferReaderConsume(TSVIOReaderGet(input_vio), towrite);
				TSVIONDoneSet(input_vio, TSVIONDoneGet(input_vio) + towrite);
			} else {
				//dbg("towrite == 0, schedule a pulse");
				TSContSchedule(contp, 100, TS_THREAD_POOL_DEFAULT);
				return;
			}
		}
	}

	if (TSVIONTodoGet(input_vio) > 0) {
		if (towrite > 0) {
			TSVIOReenable(data->output_vio);
			TSContCall(TSVIOContGet(input_vio), TS_EVENT_VCONN_WRITE_READY,
					input_vio);
		}
	} else {
		TSVIONBytesSet(data->output_vio, TSVIONDoneGet(input_vio));
		TSVIOReenable(data->output_vio);
		TSContCall(TSVIOContGet(input_vio), TS_EVENT_VCONN_WRITE_COMPLETE,
				input_vio);
	}

}

static int rate_limiting_transform(TSCont contp, TSEvent event, void *edata) {
	if (TSVConnClosedGet(contp)) {
		my_data_destroy((TransformData *) TSContDataGet(contp));
		TSContDestroy(contp);
		return 0;
	} else {
		switch (event) {
		case TS_EVENT_ERROR: {
			TSVIO input_vio = TSVConnWriteVIOGet(contp);
			TSContCall(TSVIOContGet(input_vio), TS_EVENT_ERROR, input_vio);
		}
			break;
		case TS_EVENT_VCONN_WRITE_COMPLETE:
			TSVConnShutdown(TSTransformOutputVConnGet(contp), 0, 1);
			break;
		case TS_EVENT_VCONN_WRITE_READY:
		default:
			handle_rate_limiting_transform(contp);
			break;
		}
	}

	return 0;
}

void transform_add(TSHttpTxn txnp, RateLimiter *rate_limiter) {
	TSMBuffer bufp;
	TSMLoc hdr_loc;

	if (TSHttpTxnServerRespGet(txnp, &bufp, &hdr_loc) != TS_SUCCESS) {
		return;
	}
	TSHandleMLocRelease(bufp, TS_NULL_MLOC, hdr_loc);

	TSVConn contp = TSTransformCreate(rate_limiting_transform, txnp);
	TransformData * data = my_data_alloc();
	TSDebug(PLUGIN_NAME, "my_data_alloc");
	data->rate_limiter = rate_limiter;
//	data->rate_limiter->hold();
	data->state = rate_limiter->registerLimiter();
	data->txnp = txnp;
	TSContDataSet(contp, data);
	//TSVConnActiveTimeoutCancel(contp);
	TSHttpTxnHookAdd(txnp, TS_HTTP_RESPONSE_TRANSFORM_HOOK, contp);
}

TSReturnCode txn_handler(TSCont contp, TSEvent event, void *edata) {
	TSHttpTxn txnp = (TSHttpTxn) edata;
	RateLimiter * rate_limiter =
			static_cast<RateLimiter *>(TSContDataGet(contp));
	switch (event) {
	case TS_EVENT_HTTP_READ_RESPONSE_HDR:
		TSDebug(PLUGIN_NAME, "TS_EVENT_HTTP_READ_RESPONSE_HDR!");
		transform_add(txnp, rate_limiter);
		break;
	case TS_EVENT_ERROR: {
		TSHttpTxnReenable(txnp, TS_EVENT_ERROR);
		return TS_SUCCESS;
	}
//	case TS_EVENT_HTTP_TXN_CLOSE:
//		TSDebug(PLUGIN_NAME, "TS_EVENT_HTTP_TXN_CLOSE!");
//		if (rate_limiter)
//			rate_limiter->release();
//		break;
	default:
		break;
	}
	TSHttpTxnReenable(txnp, TS_EVENT_HTTP_CONTINUE);
	return TS_SUCCESS;
}

TSReturnCode TSRemapInit(TSRemapInterface *api_info, char *errbuf,
		int errbuf_size) {
	if (!api_info) {
		snprintf(errbuf, errbuf_size,
				"[TSRemapInit] - Invalid TSRemapInterface argument");
		return TS_ERROR;
	}

	if (api_info->size < sizeof(TSRemapInterface)) {
		snprintf(errbuf, errbuf_size,
				"[TSRemapInit] - Incorrect size of TSRemapInterface structure");
		return TS_ERROR;
	}

	return TS_SUCCESS;
}

TSReturnCode TSRemapNewInstance(int argc, char *argv[], void **instance,
		char *errbuf, int errbuf_size) {
	std::map<const char *, uint64_t>::iterator it;
	uint64_t ssize;
	int i;
	char key[100];
	Configuration * conf = new Configuration();
	RateLimiter * rate_limiter = new RateLimiter();
//	rate_limiter->hold();
	ssize = 0;

	if (argc < 3) {
		TSDebug(PLUGIN_NAME, "need remap configuration file");
		goto done;
	}

//	for (int i = 2; i < argc; ++i) {
//		TSDebug(PLUGIN_NAME, "Loading remap configuration file %s", argv[i]);
//	}

	if (!conf->Parse(argv[2])) {
		goto done;
	}
	for (i = 0; i < 24; i++) {
		memset(key, '\0', sizeof(key));
		sprintf(key, "%d%s", i, "hour");
		it = conf->limitconf.find(key);
		if (it != conf->limitconf.end()) {
			ssize = it->second;
			rate_limiter->addCounter(ssize, 1000);
		} else {
			rate_limiter->addCounter(1048576, 1000);
		}
	}
	rate_limiter->setEnableLimit(true);

	done:
	delete conf;
	conf = NULL;
	*instance = rate_limiter;
	return TS_SUCCESS;
}

void TSRemapDeleteInstance(void *instance) {
	TSDebug(PLUGIN_NAME, "Delete  RateLimiter!");
//	static_cast<RateLimiter *>(instance)->release();
	RateLimiter * rate_limiter  = static_cast<RateLimiter *>(instance);
	delete rate_limiter;
}

TSRemapStatus TSRemapDoRemap(void *instance, TSHttpTxn txn,
		TSRemapRequestInfo *rri) {
	TSCont contp;

	RateLimiter * rate_limiter = static_cast<RateLimiter *>(instance);
	TSDebug(PLUGIN_NAME, "TSRemapDoRemap  getEnableLimit %d", rate_limiter->getEnableLimit());
	if (rate_limiter == NULL or !rate_limiter->getEnableLimit()) {
		return TSREMAP_NO_REMAP;
	}

	contp = TSContCreate((TSEventFunc) txn_handler, NULL);
//	rate_limiter->hold();
	TSContDataSet(contp, rate_limiter);
	TSHttpTxnHookAdd(txn, TS_HTTP_READ_RESPONSE_HDR_HOOK, contp);
	TSHttpTxnHookAdd(txn, TS_HTTP_TXN_CLOSE_HOOK, contp);

	return TSREMAP_NO_REMAP;
}

