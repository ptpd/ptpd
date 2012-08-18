/*-
 * Copyright (c) 2011-2012 George V. Neville-Neil,
 *                         Steven Kreuzer, 
 *                         Martin Burnicki, 
 *                         Jan Breuer,
 *                         Gael Mace, 
 *                         Alexandre Van Kempen,
 *                         Inaqui Delgado,
 *                         Rick Ratzel,
 *                         National Instruments,
 *                         Tomasz Kleinschmidt
 * Copyright (c) 2009-2010 George V. Neville-Neil, 
 *                         Steven Kreuzer, 
 *                         Martin Burnicki, 
 *                         Jan Breuer,
 *                         Gael Mace, 
 *                         Alexandre Van Kempen
 *
 * Copyright (c) 2005-2008 Kendall Correll, Aidan Williams
 *
 * All Rights Reserved
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE AUTHORS ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 * BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 * IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/** 
 * @file        SequnceIdConfiguration.cpp
 * @author      Tomasz Kleinschmidt
 * 
 * @brief       SequnceIdConfiguration class implementation.
 * 
 * This class is used to handle sequenceId configuration file.
 */

#include "SequnceIdConfiguration.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "MgmtMsgClient.h"

#include "constants.h"

/**
 * @brief SequnceIdConfiguration constructor.
 * 
 * @param optBuf        Buffer with application options.
 * 
 * The constructor allocates memory and trigger all necessary actions.
 */
SequnceIdConfiguration::SequnceIdConfiguration(OptBuffer* optBuf) {
    Octet *ipAddr;
    FILE *sequenceIdCfgFile = fopen (SEQUENCE_ID_CFG_FILENAME, "r");
    
    if (sequenceIdCfgFile != NULL)
    {
        Octet line [MAX_ADDR_STR_LEN * 2] = {0};
        
        while (fgets (line, sizeof(line), sequenceIdCfgFile) != NULL)
        {
            XCALLOC(ipAddr, Octet*, MAX_ADDR_STR_LEN, sizeof(Octet));
            UInteger16 seqId;
        
            if (sscanf(line, "%s %hu\n", ipAddr, &seqId) != 2) {
                WARNING("sequenceId configuration file is corrupted\n");
                break;
            }
            else
                this->sequenceIdBuf.insert(std::pair<Octet*, UInteger16>(ipAddr, seqId));
        }
        fclose(sequenceIdCfgFile);
    }
    else
        perror(SEQUENCE_ID_CFG_FILENAME);
    
    for(this->it = this->sequenceIdBuf.begin(); this->it != this->sequenceIdBuf.end(); this->it++) {
        if (!strcmp(this->it->first, optBuf->u_address)) {
            this->it->second += 1;
            optBuf->sequenceId = this->it->second;
            break;
        }
    }
    
    if (this->it == this->sequenceIdBuf.end()) {
        XCALLOC(ipAddr, Octet*, MAX_ADDR_STR_LEN, sizeof(Octet));
        memcpy(ipAddr, optBuf->u_address, strlen(optBuf->u_address));
        this->sequenceIdBuf.insert(std::pair<Octet*, UInteger16>(ipAddr, optBuf->sequenceId));
    }
}

/**
 * @brief SequnceIdConfiguration deconstructor.
 * 
 * The deconstructor stores sequenceIds in a file and frees memory.
 */
SequnceIdConfiguration::~SequnceIdConfiguration() {
    FILE *sequenceIdCfgFile = fopen (SEQUENCE_ID_CFG_FILENAME, "w");
    
    for(this->it = this->sequenceIdBuf.begin(); this->it != this->sequenceIdBuf.end(); this->it++) {
        fprintf(sequenceIdCfgFile, "%s %hu\n", this->it->first, this->it->second);
        free(this->it->first);
    }
    
    fclose(sequenceIdCfgFile);
}

