/*
 * Copyright (c) CERN 2013-2017
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>
#include <sys/time.h>
#include <pthread.h>

#include "gfal_mds_internal.h"

pthread_mutex_t m_mds =PTHREAD_MUTEX_INITIALIZER;

const char* bdii_env_var        = "LCG_GFAL_INFOSYS";
const char* bdii_config_var     = "LCG_GFAL_INFOSYS";
const char* bdii_config_group   = "BDII";
const char* bdii_config_enable  = "ENABLED";
const char* bdii_config_timeout = "TIMEOUT";

gboolean gfal_get_nobdiiG(gfal2_context_t handle)
{
    return (!gfal2_get_opt_boolean_with_default(handle, bdii_config_group, bdii_config_enable, TRUE));
}

/*
 * define the bdii endpoint for the current handle
 * same purpose that the old LCG_GFAL_INFOSYS environment variable
 */
void gfal_mds_set_infosys(gfal2_context_t handle, const char * infosys, GError** err){
	g_return_if_fail(handle && infosys);
	// no manner to define infosys in is interface currently, just setup the env var,
	// TODO : change this in is-interface and integrated module
    g_setenv(bdii_env_var, infosys, TRUE);
}

void gfal_mds_define_bdii_endpoint(gfal2_context_t handle,  GError** err){
    if(g_getenv(bdii_env_var) == NULL){
        pthread_mutex_lock(&m_mds);
        gchar * bdii_host = gfal2_get_opt_string(handle,bdii_config_group, bdii_config_var,NULL);
        if(bdii_host ){
            gfal2_log(G_LOG_LEVEL_DEBUG, " define LCG_GFAL_INFOSYS : %s", bdii_host);
            gfal_mds_set_infosys (handle, bdii_host, NULL);
        }
        g_free(bdii_host);
        pthread_mutex_unlock(&m_mds);
    }
}


/*
 * return the srm endpoints and their types, in the old way
 * */
int gfal_mds_get_se_types_and_endpoints (gfal2_context_t handle, const char *host, char ***se_types, char ***se_endpoints, GError** err){
    GError* tmp_err = NULL;
    gfal_mds_endpoint tabend[GFAL_MDS_MAX_SRM_ENDPOINT];

    int n = gfal_mds_resolve_srm_endpoint(handle, host, tabend, GFAL_MDS_MAX_SRM_ENDPOINT, &tmp_err);
    if (n > 0) {
        int i;
        *se_types = calloc(n + 1, sizeof(char*));
        *se_endpoints = calloc(n + 1, sizeof(char*));
        for (i = 0; i < n; ++i) {
            (*se_endpoints)[i] = strdup(tabend[i].url);
            (*se_types)[i] = strdup(((tabend[i].type == SRMv2) ? "srm_v2" : "srm_v1"));
        }
    }

    if (tmp_err)
        g_propagate_prefixed_error(err, tmp_err, "[%s]", __func__);
    return (n > 0) ? 0 : -1;
}


#if MDS_BDII_EXTERNAL
/*
 * external call to the is interface for external bdii resolution
 *
 */
int gfal_mds_isifce_wrapper(const char* base_url, gfal_mds_endpoint* endpoints, size_t s_endpoint, GError** err){
  char ** name_endpoints;
  char** types_endpoints;
  char errbuff[GFAL_ERRMSG_LEN]= {0};
  GError* tmp_err=NULL;
  int res = -1;


  if(sd_get_se_types_and_endpoints(base_url, &types_endpoints, &name_endpoints, errbuff, GFAL_ERRMSG_LEN-1) != 0){
    g_set_error(&tmp_err, gfal2_get_core_quark(), ENXIO, "IS INTERFACE ERROR : %s ", errbuff);
  }else{
    int i;
    char ** p1 =name_endpoints;
    char **p2 =types_endpoints;
    for(i=0; i< s_endpoint && *p1 != NULL && *p2 != NULL; ++i,++p2,++p1){
      g_strlcpy(endpoints[i].url, *p1, GFAL_URL_MAX_LEN);
      endpoints[i].type = (strcmp(*p2, "srm_v2")==0)?SRMv2:SRMv1;
    }
    res =i+1;
  }

  if(tmp_err)
    g_propagate_prefixed_error(err, tmp_err, "[%s]", __func__);
  return res;
}

#endif

 int gfal_mds_resolve_srm_endpoint(gfal2_context_t handle, const char* base_url,
         gfal_mds_endpoint* endpoints, size_t s_endpoint, GError** err)
 {
#ifndef MDS_WITHOUT_CACHE
     int cached_result = gfal_mds_cache_resolve_endpoint(handle, base_url,
                             endpoints, s_endpoint, err);
     if (cached_result < 0) {
         return cached_result;
     }
     else if (cached_result > 0) {
         int i;
         gfal2_log(G_LOG_LEVEL_DEBUG, "%s found in the cache!", base_url);
         for (i = 0; i < cached_result; ++i) {
             gfal2_log(G_LOG_LEVEL_DEBUG, "\tFound %s", endpoints[i].url);
         }
         return cached_result;
     }
#endif


#if MDS_BDII_EXTERNAL // call the is interface if configured for
    gfal_mds_define_bdii_endpoint(handle, err);
    if(err && *err==NULL)
        return gfal_mds_isifce_wrapper(base_url, endpoints, s_endpoint, err);
    return NULL;
#else
    return gfal_mds_bdii_get_srm_endpoint(handle, base_url, endpoints, s_endpoint, err);
#endif
 }





