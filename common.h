/*
 * common.h
 *
 *  Created on: 2016年9月7日
 *      Author: xie
 */

#ifndef PLUGINS_LIMIT_RATE_COMMON_H_
#define PLUGINS_LIMIT_RATE_COMMON_H_

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <map>

#define PLUGIN_NAME "limit_rate"

#define DISALLOW_COPY_AND_ASSIGN(TypeName) \
  TypeName(const TypeName&);               \
  void operator=(const TypeName&)

#endif /* PLUGINS_LIMIT_RATE_COMMON_H_ */
