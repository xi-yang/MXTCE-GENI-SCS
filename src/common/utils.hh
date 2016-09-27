/*
 * Copyright (c) 2010-2011
 * ARCHSTONE Project.
 * University of Southern California/Information Sciences Institute.
 * All rights reserved.
 *
 * Created by Xi Yang 2010
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

 
#ifndef __UTILS_HH__
#define __UTILS_HH__
 
#include "types.hh"
#include <libxml/tree.h>
#include <libxml/tree.h>
#include <libxml/xpath.h>
#include <vector>
#include <map>


#ifndef RETSIGTYPE
#define RETSIGTYPE void
#endif

using namespace std;

#define ANY_TAG 0xffff

#ifndef MAX_VLAN_NUM
#define MAX_VLAN_NUM 4096
#define VTAG_UNTAGGED MAX_VLAN_NUM
#endif

#ifndef MAX_WAVE_NUM
#define MAX_WAVE_NUM 256
#define WAVE_TUNABLE MAX_WAVE_NUM
#endif

int wavegrid_50g_to_tag(char ch, int num);

void wavegrid_tag_to_50g(char* buf, int tag);

class ConstraintTagSet
{
private:
    u_int8_t* byteArray;
    int numBits;
    int numBytes;
    bool hasAnyTag;
    int base;
    int interval;
    ConstraintTagSet() {}

public:
    ConstraintTagSet(int N) 
        { 
            numBits = N; 
            numBytes = (numBits-1)/8+1;
            byteArray = new u_int8_t[numBytes]; 
            memset(byteArray, 0, numBytes); 
            hasAnyTag = false;
            base = 1;
            interval = 1; 
        }
    ConstraintTagSet(int N, int x, int y) 
        { 
            numBits = N; 
            numBytes = (numBits-1)/8+1;
            byteArray = new u_int8_t[numBytes]; 
            memset(byteArray, 0, numBytes); 
            hasAnyTag = false;
            base = x;
            interval = y; 
        }
    ConstraintTagSet(const ConstraintTagSet& tagset) 
        { 
            byteArray = new u_int8_t[tagset.numBytes];
            memcpy(this->byteArray, tagset.byteArray, tagset.numBytes);
            numBits = tagset.numBits; 
            numBytes = tagset.numBytes;
            hasAnyTag = tagset.hasAnyTag;
            base = tagset.base;
            interval = tagset.interval; 
        }
    ~ConstraintTagSet() 
        { 
            delete []byteArray;
        }
    ConstraintTagSet& operator= (const ConstraintTagSet& tagset)
        {
            if (this->numBytes != tagset.numBytes)
            {
                delete []byteArray;
                byteArray = new u_int8_t[tagset.numBytes];
            }
            memcpy(this->byteArray, tagset.byteArray, tagset.numBytes);
            numBits = tagset.numBits; 
            numBytes = tagset.numBytes;
            hasAnyTag = tagset.hasAnyTag;
            base = tagset.base;
            interval = tagset.interval;
            return *this;
        }
    void AddTag(u_int32_t tag)
        {
            if (tag == ANY_TAG)
                hasAnyTag = true;
            else
            {
                tag = (tag-base)/interval;
                byteArray[tag/8] |= (0x80>>(tag%8));
            }
        }
    void AddTags(int num, u_int32_t* tags)
        {
            assert(tags);
            for (int i = 0; i < num; i++)
                AddTag(tags[i]);
        }
    void AddTags(int num, u_int16_t* tags)
        {
            assert(tags);
            for (int i = 0; i < num; i++)
                AddTag((u_int32_t)tags[i]);
        }
    void AddTags(u_char* bitmask, int max_num)
        {
            assert(bitmask);
            for (int i = 0; i < (max_num-1)/8+1 && i < numBytes; i++)
                byteArray[i] |= bitmask[i];
        }
    void LoadRangeString(string rangeStr)
        {
            char buf[1024];
            strncpy(buf, rangeStr.c_str(), 256);
            if (strncasecmp(buf, "any", 3) == 0)
            {                
                AddTag(ANY_TAG);
                return;
            }
            if (strncasecmp(buf, "untagged", 5) == 0)
            {                
                AddTag(VTAG_UNTAGGED);
                return;
            }
            for (char* ptr = strtok(buf, ", \t\n"); ptr; ptr = strtok(NULL,  ", \t\n"))
            {
                int low, high;
                int n = sscanf(ptr, "%d-%d", &low, &high);
                if (n == 1) 
                {
                    if (low <= 0 && numBits == MAX_VLAN_NUM)
                        AddTag(VTAG_UNTAGGED);
                    else
                        AddTag(low);
                }
                else if (n == 2)
                {
                    for (int t = low; t <= high; t++)
                        AddTag(t);
                }
            }
        }
    void LoadRangeString_WaveGrid_50GHz(string rangeStr)
        {
            char buf[1024];
            strncpy(buf, rangeStr.c_str(), 256);
            if (strncasecmp(buf, "tunable", 5) == 0 || strncasecmp(buf, "any", 3) == 0)
            {                
                AddTag(ANY_TAG);
                return;
            }
            for (char* ptr = strtok(buf, ", \t\n"); ptr; ptr = strtok(NULL,  ", \t\n"))
            {
                char cl, ch;
                int low, high;
                int n = sscanf(ptr, "%c%d-%c%d", &cl, &low, &ch, &high);
                if (n == 2) 
                {
                    AddTag(wavegrid_50g_to_tag(cl, low));
                }
                else if (n == 4)
                {
                    low = wavegrid_50g_to_tag(cl, low);
                    high = wavegrid_50g_to_tag(ch, high); 
                    for (int t = low; t <= high; t++)
                        AddTag(t);
                }
            }
        }
    void DeleteTag(u_int32_t tag)
        {
            if (tag == ANY_TAG)
                hasAnyTag = false;
            else
            {
                tag = (tag-base)/interval;
                byteArray[tag/8] &= (~(0x80>>(tag%8)));
            }
        }
    void DeleteTags(int num, u_int32_t* tags)
        {
            assert(tags);
            for (int i = 0; i < num; i++)
                DeleteTag(tags[i]);
        }
    void DeleteTags(int num, u_int16_t* tags)
        {
            assert(tags);
            for (int i = 0; i < num; i++)
                DeleteTag((u_int32_t)tags[i]);
        }
    void DeleteTags(u_char* bitmask, int max_num)
        {
            assert(bitmask);
            for (int i = 0; i < (max_num-1)/8+1 && i < numBytes; i++)
                byteArray[i] &= (~bitmask[i]);
        }
    bool HasTag(u_int32_t tag)
        {
            if (tag == ANY_TAG)
                return (!IsEmpty());
            tag = (tag-base)/interval;
            if (tag >= numBits)
                return false;
            return (byteArray[tag/8]&(0x80>>(tag%8)));
        }
    bool HasAnyTag() { return hasAnyTag; }
    bool IsEmpty()
        {
            if (hasAnyTag)
                return false;

            for (int i = 0; i < numBytes; i++)
                if (byteArray[i] != 0)
                    return false;

            return true;
        }
    void Intersect(ConstraintTagSet & tagset)
        {
            assert(this->base == tagset.base && this->interval == tagset.interval);
            if (tagset.HasAnyTag())
                return;
            if (this->HasAnyTag())
            {
                memcpy(this->byteArray, tagset.byteArray, this->numBytes);
                this->hasAnyTag = false;
                return;
            }
            for (int i = 0; i < this->numBytes && i < tagset.numBytes; i++)
                this->byteArray[i] &= tagset.byteArray[i];
            hasAnyTag = false;
        }
    void Join(ConstraintTagSet & tagset)
        {
            assert(this->base == tagset.base && this->interval == tagset.interval);
            if (this->hasAnyTag)
                return;
            if (tagset.HasAnyTag()) 
            {
                memset(this->byteArray, this->numBytes, 0xff);
                this->hasAnyTag = true;
                return;
            }
            for (int i = 0; i < this->numBytes && i < tagset.numBytes; i++)
                this->byteArray[i] |= tagset.byteArray[i];
        }
    u_int32_t LowestTag()
        {
            for (int i = 0; i < numBytes; i++)
                if (byteArray[i] != 0)
                {
                    if (byteArray[i]&0x80)
                        return i*8*interval + base;
                    else if (byteArray[i]&0x40)
                        return (i*8+1)*interval + base;
                    else if (byteArray[i]&0x20)
                        return (i*8+2)*interval + base;
                    else if (byteArray[i]&0x10)
                        return (i*8+3)*interval + base;
                    else if (byteArray[i]&0x08)
                        return (i*8+4)*interval + base;
                    else if (byteArray[i]&0x04)
                        return (i*8+5)*interval + base;
                    else if (byteArray[i]&0x02)
                        return (i*8+6)*interval + base;
                    else
                        return (i*8+7)*interval + base;
                }
            return 0;
        }
    u_int32_t HighestTag()
        {
            for (int i = numBytes-1; i >=0; i--)
                if (byteArray[i] != 0)
                {
                    if (byteArray[i]&0x01)
                        return (i*8+7)*interval+base;
                    else if (byteArray[i]&0x02)
                        return (i*8+6)*interval+base;
                    else if (byteArray[i]&0x04)
                        return (i*8+5)*interval+base;
                    else if (byteArray[i]&0x08)
                        return (i*8+4)*interval+base;
                    else if (byteArray[i]&0x10)
                        return (i*8+3)*interval+base;
                    else if (byteArray[i]&0x20)
                        return (i*8+2)*interval+base;
                    else if (byteArray[i]&0x40)
                        return (i*8+1)*interval+base;
                    else
                        return i*8*interval+base;
                }
            return 0;
        }
    u_int32_t RandomTag(int low, int high)
        {
            if (low == 0)
                return 0;
            if (low == high)
                return low;
            int start = low + (int)(random()% (high-low));
            int tag;
            if (start%2 == 0)
            {
                for (tag = start; tag < numBits; tag++)
                {
                    if (HasTag(tag))
                        return tag;
                }
            }
            else
            {
                for (tag = start; tag >= 0; tag--)
                {
                    if (HasTag(tag))
                        return tag;
                }
            }
            return 0;
        }
    u_int32_t RandomTag()
        {
            // TODO: store last five tags as global, try making a new number
            if (this->hasAnyTag)
                return (u_int32_t)((random() % (this->numBits-1))+1);
            int low = this->LowestTag();
            int high = this->HighestTag();
            int tag = RandomTag(low, high);
            if (HasTag(tag+1) && tag < high)
                return RandomTag(tag+1, high);
            else
                return RandomTag(low, tag);                
        }
    int Size() 
        {
            int num = 0;
            for (int i = numBytes-1; i >=0; i--)
                if (byteArray[i] != 0)
                {
                    if (byteArray[i]&0x01)
                        num++;
                    if (byteArray[i]&0x02)
                        num++;
                    if (byteArray[i]&0x04)
                        num++;
                    if (byteArray[i]&0x08)
                        num++;
                    if (byteArray[i]&0x10)
                        num++;
                    if (byteArray[i]&0x20)
                        num++;
                    if (byteArray[i]&0x40)
                        num++;
                    if (byteArray[i]&0x80)
                        num++;
                }
            return num;
        }
    void Clear() { memset(byteArray, 0, numBytes); this->hasAnyTag = false; }
    u_int8_t* TagBitmask() { return byteArray; }
    string GetRangeString() 
        { 
            string rangeStr = "";
            if (this->hasAnyTag)
            {
                rangeStr = "any";
                return rangeStr;
            }
            char buf[1024];
            char buf2[32];
            if (numBits == MAX_VLAN_NUM && HasTag(VTAG_UNTAGGED))
                strcpy(buf, "0");
            else
                buf[0] = 0;
            int i, i_start, i_end;
            for (i = 1, i_start = 1, i_end = 0; i <= numBits; i++)
            {
                if (numBits == MAX_VLAN_NUM && i == numBits)
                    break;
                if (HasTag(i))
                {
                    if (i_end < i_start)
                    {
                        sprintf(buf2, ",%d", i);
                        strcat(buf, buf2);
                        i_start = i;
                    }
                    if (i == numBits)
                    {
                        if (i - i_start >= 2) 
                        {
                            sprintf(buf2, "-%d", i);
                            strcat(buf, buf2);
                        }
                        else if (i - i_start == 1) 
                        {
                            sprintf(buf2,",%d", i);
                            strcat(buf, buf2);
                        }
                    }
                    i_end = i;
                }
                else
                {
                    if (i_end - i_start >= 2)
                    {
                        sprintf(buf2, "-%d", i_end);
                        strcat(buf, buf2);
                    }
                    else if (i_end - i_start == 1)
                    {
                        sprintf(buf2, ",%d", i_end);
                        strcat(buf, buf2);
                    }
                    i_end = 0;
                }
            }
            if (buf[0] == ',')
                rangeStr = (const char*)buf+1;
            else 
                rangeStr = (const char*)buf;
            return rangeStr;
        }
        string GetRangeString_WaveGrid_50GHz() 
            { 
                string rangeStr = "";
                char buf[1024];
                char buf2[32];
                char buf3[8];
                if (numBits == MAX_WAVE_NUM && HasTag(WAVE_TUNABLE))
                {
                    strcpy(buf, "tunable");
                    rangeStr = (const char*)buf;
                    return rangeStr;
                }
                else
                    buf[0] = 0;
                int i, i_start, i_end;
                for (i = 1, i_start = 1, i_end = 0; i <= numBits; i++)
                {
                    if (numBits == MAX_VLAN_NUM && i == numBits)
                        break;
                    if (HasTag(i))
                    {
                        if (i_end < i_start)
                        {
                            wavegrid_tag_to_50g(buf3, i);
                            sprintf(buf2, ",%s", buf3);
                            strcat(buf, buf2);
                            i_start = i;
                        }
                        if (i == numBits)
                        {
                            if (i - i_start >= 2) 
                            {
                                wavegrid_tag_to_50g(buf3, i);
                                sprintf(buf2, "-%s", buf3);
                                strcat(buf, buf2);
                            }
                            else if (i - i_start == 1) 
                            {
                                wavegrid_tag_to_50g(buf3, i);
                                sprintf(buf2,",%s", buf3);
                                strcat(buf, buf2);
                            }
                        }
                        i_end = i;
                    }
                    else
                    {
                        if (i_end - i_start >= 2)
                        {
                            wavegrid_tag_to_50g(buf3, i_end);
                            sprintf(buf2, "-%s", buf3);
                            strcat(buf, buf2);
                        }
                        else if (i_end - i_start == 1)
                        {
                            wavegrid_tag_to_50g(buf3, i_end);
                            sprintf(buf2, ",%s", buf3);
                            strcat(buf, buf2);
                        }
                        i_end = 0;
                    }
                }
                if (buf[0] == ',')
                    rangeStr = (const char*)buf+1;
                else 
                    rangeStr = (const char*)buf;
                return rangeStr;
            }
    };


extern "C"
{
    /* Read nbytes from fd and store into ptr. */
    int readn (int fd, char *ptr, int nbytes);
    
    /* Write nbytes from ptr to fd. */
    int writen(int fd, char *ptr, int nbytes);

    RETSIGTYPE * signal_set (int signo, void (*func)(int));
    
    void signal_init ();

    int mkpath(const char *path, mode_t mode);

    time_t get_mtime(const char *path);
}

const char* getVersionString();

void SplitString(string& str, vector<string>& tokens, const string& delim=" ", bool trimEmpty=false);

void StripXmlString(string& str, xmlChar* val);
void CleanupXmlString(string& str);
void RemoveXmlNsPrefix(string& str);

u_int64_t StringToBandwidth(string& strBandwidth, u_int64_t defaultFactor=1LLU);

string GetUrnField(string& urn, const char* field);

string ConvertLinkUrn_Dnc2Geni(string& urn);

string GetUrnFieldExt(string& urn, const char* field);

string ConvertLinkUrn_Dnc2GeniExt(string& urn);

void ParseFQUrn(string& urn, string& domain, string& node, string& port, string& link);

void ParseFQUrnShort(string& urn, string& domain, string& node, string& port, string& link);

xmlNodeSetPtr GetXpathNodeSet (xmlDocPtr doc, const char *xpath, map<string, string>* nsMap=NULL);

xmlNodePtr GetXpathNode (xmlDocPtr doc, const char *xpath, map<string, string>* nsMap=NULL);

#endif
