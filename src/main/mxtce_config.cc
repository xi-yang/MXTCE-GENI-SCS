/*
 * Copyright (c) 2010-2011
 * ARCHSTONE Project.
 * University of Southern California/Information Sciences Institute.
 * All rights reserved.
 *
 * Created by Xi Yang 2011
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the project nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE PROJECT AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE PROJECT OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <yaml.h>
#include "mxtce_config.hh"

void MxTCEConfig::ParseYamlConfig()
{
    yaml_parser_t parser;
    yaml_document_t document;    
    yaml_parser_initialize(&parser);
    FILE *file = fopen(configFilePath.c_str(), "rb");
    if (file == NULL)
        throw TCEException((char*)"Cannot open config file.");
    yaml_parser_set_input_file(&parser, file);
    if (!yaml_parser_load(&parser,&document))
        throw TCEException((char*)"Failed to parse yaml.");
    yaml_node_t * rootNode = yaml_document_get_root_node(&document);
    if (rootNode == NULL || rootNode->type != YAML_MAPPING_NODE) 
        throw TCEException((char*)"Improperly formatted configuration file!");
    for (yaml_node_pair_t * i = rootNode->data.mapping.pairs.start; i < rootNode->data.mapping.pairs.top; ++i)
    {
        yaml_node_t * nodeL1key = yaml_document_get_node(&document, i->key);
        yaml_node_t * nodeL1val = yaml_document_get_node(&document, i->value);
        if (nodeL1val == NULL)
            throw TCEException((char*)"Improperly formatted configuration file!");
        if (nodeL1val->type == YAML_SCALAR_NODE) 
        {
            ;
        }
        else if (nodeL1val->type == YAML_MAPPING_NODE) 
        {
            for (yaml_node_pair_t * j = nodeL1val->data.mapping.pairs.start; j < nodeL1val->data.mapping.pairs.top; ++j)
            {
                yaml_node_t * nodeL2key = yaml_document_get_node(&document, j->key);
                yaml_node_t * nodeL2val = yaml_document_get_node(&document, j->value);
                if (nodeL2val == NULL) 
                    throw TCEException((char*)"Improperly formatted configuration file!");    
                if (nodeL2val->type == YAML_SCALAR_NODE) 
                {
                    ParseLevel2Config((char*)nodeL1key->data.scalar.value, (char*)nodeL2key->data.scalar.value, (char*)nodeL2val->data.scalar.value);
                }
                else if (nodeL2val->type == YAML_MAPPING_NODE) 
                {
                    for (yaml_node_pair_t * k = nodeL2val->data.mapping.pairs.start; k < nodeL2val->data.mapping.pairs.top; ++k)
                    {
                        yaml_node_t * nodeL3key = yaml_document_get_node(&document, k->key);
                        yaml_node_t * nodeL3val = yaml_document_get_node(&document, k->value);
                        if (nodeL3val == NULL)
                            throw TCEException((char*)"Improperly formatted configuration file!");    
                        if (nodeL3val->type == YAML_SCALAR_NODE) 
                        {
                            ParseLevel3Config((char*)nodeL1key->data.scalar.value, (char*)nodeL2key->data.scalar.value, 
                                (char*)nodeL3key->data.scalar.value, (char*)nodeL3val->data.scalar.value);
                        }
                        else if (nodeL3val->type == YAML_MAPPING_NODE) 
                        {
                            for (yaml_node_pair_t * l = nodeL3val->data.mapping.pairs.start; l < nodeL3val->data.mapping.pairs.top; ++l)
                            {
                                yaml_node_t * nodeL4key = yaml_document_get_node(&document, l->key);
                                yaml_node_t * nodeL4val = yaml_document_get_node(&document, l->value);
                                if (nodeL4val == NULL)
                                    throw TCEException((char*)"Improperly formatted configuration file!");    
                                if (nodeL4val->type == YAML_SCALAR_NODE) 
                                {
                                    ParseLevel4Config((char*)nodeL1key->data.scalar.value, (char*)nodeL2key->data.scalar.value, 
                                        (char*)nodeL3key->data.scalar.value, (char*)nodeL4key->data.scalar.value, (char*)nodeL4val->data.scalar.value);
                                }
                                else
                                    throw TCEException((char*)"Improperly formatted configuration file!");    
                            }
                        }
                        else 
                            throw TCEException((char*)"Improperly formatted configuration file!");
                    }
                }
                else 
                    throw TCEException((char*)"Improperly formatted configuration file!");
            }
        }
        else
            throw TCEException((char*)"Improperly formatted configuration file!");
    }
    yaml_parser_delete(&parser);
    yaml_document_delete(&document);
    fclose(file);
    // TODO: verify configs
}

void MxTCEConfig::ParseLevel2Config(char* key1, char* key2, char* val)
{
    if (strcasecmp(key1, "apiServer") == 0)
    {
        if (strcasecmp(key2, "port") == 0)
        {
            if (sscanf(val, "%d", &MxTCE::apiServerPort) != 1)
                throw TCEException((char*)"Error in configuration file (apiServer / port)!");
        }
        else if (strcasecmp(key2, "timeout") == 0)
        {
            if (sscanf(val, "%d", &APIServer::maxApiTimeOutSecs) != 1)
                throw TCEException((char*)"Error in configuration file (apiServer / timeout)!");
        }
        else if (strcasecmp(key2, "client") == 0)
        {
            ; // place holder
        }
    }
    else if (strcasecmp(key1, "computeWorker") == 0)
    {
        if (strcasecmp(key2, "type") == 0)
        {
            MxTCE::defaultComputeWorkerType = val;
        }
        if (strcasecmp(key2, "exclusiveConcurrent") == 0)
        {
            if (strcasecmp(val, "true") == 0) 
                MxTCE::exclusiveConcurrentHolding = true;
        }
    }
}

void MxTCEConfig::ParseLevel3Config(char* key1, char* key2, char* key3, char* val)
{
    if (strcasecmp(key1, "resvManager") == 0)
    {
        if (strcasecmp(key2, "pushApiServer") == 0)
        {
            if (strcasecmp(key3, "port") == 0)
            {
                if (sscanf(val, "%d", &MxTCE::resvApiServerPort) != 1)
                    throw TCEException((char*)"Error in configuration file (resvManager / pushApiServer / port)!");
            }
            else if (strcasecmp(key3, "client") == 0)
            {
                ; // place holder
            }
        }
    }
}


void MxTCEConfig::ParseLevel4Config(char* key1, char* key2, char* key3, char* key4, char* val)
{
    if (strcasecmp(key1, "tedbManager") == 0)
    {
        if (strcasecmp(key2, "domains") == 0)
        {
            if (strcasecmp(key4, "type") == 0)
            {
                string domainId = key3;
                string domainType = val;
                // TODO: create a domain-type map ?
            }
            else if (strcasecmp(key4, "file") == 0)
            {
                string domainFile = val;
                MxTCE::xmlDomainFileList.push_back(domainFile);
            }
        }
    }
}

