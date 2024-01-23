/*
 * Copyright (c) CERN 2022
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

#include <time.h>
#include <netdb.h>
#include <arpa/inet.h>

#include "gfal2_network.h"
#include "gfal_plugins_api.h"
#include <uri/gfal2_uri.h>

char* resolve_dns_helper(const char* host_uri, const char* msg)
{
    char* resolved_str;
    GError *error = NULL;
    gfal2_uri *parsed = gfal2_parse_uri(host_uri, &error);

    if (error) {
        gfal2_log(G_LOG_LEVEL_WARNING, "Failed to parse host uri while resolving DNS alias: %s", host_uri);
        return NULL;
    }

    char *resolved = gfal2_resolve_dns_to_hostname(parsed->host);

    if (!resolved) {
        return NULL;
    }

    gfal2_log(G_LOG_LEVEL_INFO, "%s: %s => %s", msg, parsed->host, resolved);
    g_free(parsed->host);
    parsed->host = resolved;
    resolved_str = gfal2_join_uri(parsed);
    gfal2_free_uri(parsed);
    return resolved_str;
}

char* gfal2_resolve_dns_to_hostname(const char* dnshost)
{
    struct addrinfo hints;
    struct addrinfo* addresses = NULL;
    struct addrinfo* addrP = NULL;
    GString* log_str = g_string_sized_new(512);
    char addrstr[INET6_ADDRSTRLEN * 2];
    char hostname[256];
    void* ptr = NULL;
    int count = 0;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags |= AI_CANONNAME;

    int rc = getaddrinfo(dnshost, NULL, &hints, &addresses);

    if (rc || !addresses) {
        if (addresses) {
            freeaddrinfo(addresses);
        }

        gfal2_log(G_LOG_LEVEL_WARNING, "Could not resolve DNS alias: %s", dnshost);
        return NULL;
    }

    // Count and log all resolved addresses
    for (addrP = addresses; addrP != NULL; addrP = addrP->ai_next) {
        inet_ntop(addrP->ai_family, addrP->ai_addr->sa_data, addrstr, sizeof(addrstr));

        switch (addrP->ai_family) {
            case AF_INET:
                ptr = &((struct sockaddr_in *) addrP->ai_addr)->sin_addr;
                if (ptr) {
                    inet_ntop(addrP->ai_family, ptr, addrstr, sizeof(addrstr));
                }
                break;
            case AF_INET6:
                ptr = &((struct sockaddr_in6 *) addrP->ai_addr)->sin6_addr;
                if (ptr) {
                    inet_ntop(addrP->ai_family, ptr, addrstr, sizeof(addrstr));
                }
                break;
        }

        // Reverse DNS. Try to translate the address to a hostname. If successful save hostname for logging
        if (getnameinfo(addrP->ai_addr, addrP->ai_addrlen, hostname, sizeof(hostname),
                        NULL, 0, NI_NAMEREQD)) {
            gfal2_log(G_LOG_LEVEL_WARNING, "Failed reverse address %s into hostname", addrstr);
        } else {
            g_string_append_printf(log_str, "%s[%s] ", hostname, addrstr);
        }

        count++;
    }

    gfal2_log(G_LOG_LEVEL_DEBUG, "Resolved DNS alias %s into: %s", dnshost, log_str->str);
    g_string_free(log_str, TRUE);

    // Select at random an address between [0, count)
    srand(time(NULL));
    int selected = rand() % count;

    for (addrP = addresses; addrP != NULL; addrP = addrP->ai_next) {
        if (selected-- == 0) {
            if (getnameinfo(addrP->ai_addr, addrP->ai_addrlen, hostname, sizeof(hostname),
                            NULL, 0, NI_NAMEREQD)) {
                freeaddrinfo(addresses);
                gfal2_log(G_LOG_LEVEL_WARNING, "Failed reverse DNS resolution for %s ", dnshost);
                return NULL;
            }

            break;
        }
    }

    freeaddrinfo(addresses);
    return strdup(hostname);
}
