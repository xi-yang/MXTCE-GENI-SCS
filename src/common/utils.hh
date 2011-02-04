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


#ifndef RETSIGTYPE
#define RETSIGTYPE void
#endif

#define ANY_TAG 0xffff

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
            for (int i = 1; i < (max_num-1)/8+1 && i < numBytes; i++)
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
            for (int i = 0; i < this->numBytes && i < tagset.numBytes; i++)
                this->byteArray[i] &= tagset.byteArray[i];
            hasAnyTag = false;
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
    void Clear() { memset(byteArray, 0, numBytes); }
    u_int8_t* TagBitmask() { return byteArray; }
    /*
    void DisplayTags()
        {
            if (IsEmpty()) return;
            cout << "Tags:";
            for (u_int32_t i = 0; i < numBits; i++)
                if (HasTag(i))
                    cout << ' ' << (i-1)*interval+base;
            cout << endl;
        }
        */
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

    bool float_zero();
}


#endif
