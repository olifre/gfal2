/*
 * Copyright (c) CERN 2013-2015
 *
 * Copyright (c) Members of the EMI Collaboration. 2010-2013
 *  See  http://www.eu-emi.eu/partners for details on the copyright
 *  holders.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once
#ifndef GFAL_CONFIG_INTERNAL_H_
#define GFAL_CONFIG_INTERNAL_H_

#include <common/gfal_prototypes.h>
#include <common/gfal_types.h>
#include <config/gfal_config.h>

// create or delete configuration manager for gfal 2.0, internal
gfal_conf_t gfal_conf_new();

void gfal_conf_delete(gfal_conf_t conf);

GQuark gfal_quark_config_loader();

void gfal_free_keyvalue(gpointer data, gpointer user_data);

#endif /* GFAL_CONFIG_INTERNAL_H_ */