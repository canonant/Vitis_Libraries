/*
 * (c) Copyright 2019 Xilinx, Inc. All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */
#ifndef _XFCOMPRESSION_LZ4_DECOMPRESS_HPP_
#define _XFCOMPRESSION_LZ4_DECOMPRESS_HPP_

/**
 * @file lz4_decompress.hpp
 * @brief Header for modules used in LZ4 decompression kernel.
 *
 * This file is part of Vitis Data Compression Library.
 */
#include "hls_stream.h"
#include "lz_decompress.hpp"

#include <ap_int.h>
#include <assert.h>
#include <stdint.h>
#include <stdio.h>

namespace xf {
namespace compression {

typedef ap_uint<32> compressd_dt;

/**
 * @brief This module reads the compressed data from input stream
 * and decodes the offset, match length and literals by processing
 * in various decompress states.
 *
 *
 * @param inStream Input stream 8bit
 * @param outStream Output stream 32bit
 * @param input_size Input size
 */
inline void lz4Decompress(hls::stream<ap_uint<8> >& inStream,
                          hls::stream<compressd_dt>& outStream,
                          uint32_t input_size) {
    enum lz4DecompressStates { READ_TOKEN, READ_LIT_LEN, READ_LITERAL, READ_OFFSET0, READ_OFFSET1, READ_MATCH_LEN };
    enum lz4DecompressStates next_state = READ_TOKEN;
    ap_uint<8> nextValue;
    ap_uint<16> offset;
    compressd_dt decompressdOut = 0;
    uint32_t lit_len = 0;
    uint32_t match_len = 0;
lz4_decompressr:
    for (uint32_t i = 0; i < input_size; i++) {
#pragma HLS PIPELINE II = 1
        ap_uint<8> inValue = inStream.read();
        if (next_state == READ_TOKEN) {
            lit_len = inValue.range(7, 4);
            match_len = inValue.range(3, 0);
            if (lit_len == 0xF) {
                next_state = READ_LIT_LEN;
            } else if (lit_len) {
                next_state = READ_LITERAL;
            } else {
                next_state = READ_OFFSET0;
            }
        } else if (next_state == READ_LIT_LEN) {
            lit_len += inValue;
            if (inValue != 0xFF) {
                next_state = READ_LITERAL;
            }
        } else if (next_state == READ_LITERAL) {
            compressd_dt outValue = 0;
            outValue.range(7, 0) = inValue;
            outStream << outValue;
            lit_len--;
            if (lit_len == 0) {
                next_state = READ_OFFSET0;
            }
        } else if (next_state == READ_OFFSET0) {
            offset.range(7, 0) = inValue;
            next_state = READ_OFFSET1;
        } else if (next_state == READ_OFFSET1) {
            offset.range(15, 8) = inValue;
            if (match_len == 0xF) {
                next_state = READ_MATCH_LEN;
            } else {
                next_state = READ_TOKEN;
                compressd_dt outValue = 0;
                outValue.range(31, 16) = (match_len + 3); //+3 for LZ4 standard
                outValue.range(15, 0) = (offset - 1);     //-1 for LZ4 standard
                outStream << outValue;
            }
        } else if (next_state == READ_MATCH_LEN) {
            match_len += inValue;
            if (inValue != 0xFF) {
                compressd_dt outValue = 0;
                outValue.range(31, 16) = (match_len + 3); //+3 for LZ4 standard
                outValue.range(15, 0) = (offset - 1);     //-1 for LZ4 standard
                outStream << outValue;
                next_state = READ_TOKEN;
            }
        }
    }
}

inline void lz4DecompressSimple(hls::stream<ap_uint<8> >& inStream,
                                hls::stream<compressd_dt>& outStream,
                                uint32_t input_size,
                                bool uncomp_flag) {
    enum lz4DecompressStates { READ_TOKEN, READ_LIT_LEN, READ_LITERAL, READ_OFFSET0, READ_OFFSET1, READ_MATCH_LEN };
    enum lz4DecompressStates next_state = READ_TOKEN;
    ap_uint<8> nextValue;
    ap_uint<16> offset;
    compressd_dt decompressdOut = 0;
    uint32_t lit_len = 0;
    uint32_t match_len = 0;
    if (uncomp_flag == 1) {
        next_state = READ_LITERAL;
        lit_len = input_size;
    }
lz4_decompressr:
    for (uint32_t i = 0; i < input_size; i++) {
#pragma HLS PIPELINE II = 1
        ap_uint<8> inValue = inStream.read();
        if (next_state == READ_TOKEN) {
            lit_len = inValue.range(7, 4);
            match_len = inValue.range(3, 0);
            if (lit_len == 0xF) {
                next_state = READ_LIT_LEN;
            } else if (lit_len) {
                next_state = READ_LITERAL;
            } else {
                next_state = READ_OFFSET0;
            }
        } else if (next_state == READ_LIT_LEN) {
            lit_len += inValue;
            if (inValue != 0xFF) {
                next_state = READ_LITERAL;
            }
        } else if (next_state == READ_LITERAL) {
            compressd_dt outValue = 0;
            outValue.range(7, 0) = inValue;
            outStream << outValue;
            lit_len--;
            if (lit_len == 0) {
                next_state = READ_OFFSET0;
            }
        } else if (next_state == READ_OFFSET0) {
            offset.range(7, 0) = inValue;
            next_state = READ_OFFSET1;
        } else if (next_state == READ_OFFSET1) {
            offset.range(15, 8) = inValue;
            if (match_len == 0xF) {
                next_state = READ_MATCH_LEN;
            } else {
                next_state = READ_TOKEN;
                compressd_dt outValue = 0;
                outValue.range(31, 16) = (match_len + 3); //+3 for LZ4 standard
                outValue.range(15, 0) = (offset - 1);     //-1 for LZ4 standard
                outStream << outValue;
            }
        } else if (next_state == READ_MATCH_LEN) {
            match_len += inValue;
            if (inValue != 0xFF) {
                compressd_dt outValue = 0;
                outValue.range(31, 16) = (match_len + 3); //+3 for LZ4 standard
                outValue.range(15, 0) = (offset - 1);     //-1 for LZ4 standard
                outStream << outValue;
                next_state = READ_TOKEN;
            }
        }
    }
}

/**
 * @brief This module reads the compressed data from input stream
 * and decodes the offset, match length and literals by processing
 * in various decompress states. It can process the data in parallel
 * defined by PARALLEL BYTES template parameter. 
 *
 * @tparam PARALLEL_BYTES number of bytes processed in parallel (4,8)
 *
 * @param inStream input stream
 * @param litlenStream literal length stream
 * @param litStream literal stream
 * @param offsetStream offset stream
 * @param matchlenStream match length only stream
 * @param input_size input size of the block
 */
template <int PARALLEL_BYTES>
inline void lz4MultiByteDecompress(hls::stream<ap_uint<PARALLEL_BYTES * 8> >& inStream,
                                   hls::stream<uint16_t>& litlenStream,
                                   hls::stream<ap_uint<PARALLEL_BYTES * 8> >& litStream,
                                   hls::stream<ap_uint<16> >& offsetStream,
                                   hls::stream<uint16_t>& matchlenStream,
                                   uint32_t input_size) {
    if (input_size == 0) return;

    enum lz4DecompressStates { READ_TOKEN, READ_LIT_LEN, READ_LITERAL, READ_OFFSET, READ_MATCH_LEN };
    enum lz4DecompressStates next_state = READ_TOKEN;

    const int c_parallelBit = PARALLEL_BYTES * 8;
    uint8_t token_match_len = 0;
    uint8_t token_lit_len = 0;
    uint8_t input_index = 0;
    int8_t output_index = 0;
    uint32_t lit_len = 0;
    uint32_t match_len = 0;
    uint32_t out_written = 0;
    bool outFlag;

    ap_uint<16> offset;
    ap_uint<c_parallelBit> outStreamValue;
    ap_uint<c_parallelBit> inValue;

    ap_uint<3 * PARALLEL_BYTES * 8> input_window;
    ap_uint<2 * PARALLEL_BYTES * 8> output_window;

    // Pre-read two data from the stream (two based on the READ_TOKEN)
    for (int i = 0; i < 2; i++) {
#pragma HLS PIPELINE II = 1
        inValue = inStream.read();
        input_window.range(((i + 1) * c_parallelBit) - 1, i * c_parallelBit) = inValue;
    }

    // Initialize the loop readBytes variable to input_window buffer size as
    // that much data is already read from stream
    uint32_t readBytes = 2 * PARALLEL_BYTES;
    uint32_t processedBytes = 0;
lz4_decompressr:
    for (; ((processedBytes + input_index) < input_size);) {
// printf("input_size=%d readBytes=%d\n",input_size, readBytes);
#pragma HLS PIPELINE II = 1
        //  then shift the input buffer data and read c_parallelBit data
        //  from inStream and put the data in input buffer
        // printf("input_index=%d, output_index=%d\n",input_index,output_index);
        if ((input_index >= PARALLEL_BYTES) & (readBytes < input_size)) {
            input_window >>= c_parallelBit;

            ap_uint<c_parallelBit> input = inStream.read();
            readBytes += PARALLEL_BYTES;
            input_window.range(2 * c_parallelBit - 1, c_parallelBit) = input.range(c_parallelBit - 1, 0);
            input_index = input_index - PARALLEL_BYTES;
            processedBytes += PARALLEL_BYTES;
        }

        // READ TOKEN stage
        if (next_state == READ_TOKEN) {
            // printf("READ_TOKEN\n");
            ap_uint<8> token_value = input_window.range((input_index + 1) * 8 - 1, input_index * 8);
            token_lit_len = token_value.range(7, 4);
            token_match_len = token_value.range(3, 0);
            bool c0 = (token_lit_len == 0xF);
            input_index += 1;
            lit_len = token_lit_len;

            if (c0) {
                next_state = READ_LIT_LEN;
            } else if (lit_len) {
                next_state = READ_LITERAL;
                litlenStream << lit_len;
            } else {
                next_state = READ_OFFSET;
                litlenStream << lit_len;
            }
            // printf("TokenLitlen: %d\n",lit_len);
        } else if (next_state == READ_LIT_LEN) {
            // printf("READ_LIT_LEN\n");
            ap_uint<8> token_value = input_window.range((input_index + 1) * 8 - 1, input_index * 8);
            input_index += 1;
            lit_len += token_value;

            if (token_value == 0xFF) {
                next_state = READ_LIT_LEN;
            } else {
                next_state = READ_LITERAL;
                litlenStream << lit_len;
            }
            // printf("Litlen: %d, next_state: %d\n",lit_len,next_state);
        } else if (next_state == READ_LITERAL) {
            // printf("READ_LITERAL\n");
            output_window.range((output_index + PARALLEL_BYTES) * 8 - 1, output_index * 8) =
                input_window.range((input_index + PARALLEL_BYTES) * 8 - 1, input_index * 8);
            if (lit_len >= PARALLEL_BYTES) {
                output_index += PARALLEL_BYTES;
                input_index += PARALLEL_BYTES;
                lit_len -= PARALLEL_BYTES;
            } else {
                output_index += lit_len;
                input_index += lit_len;
                lit_len = 0;
            }
            if (lit_len == 0) {
                next_state = READ_OFFSET;
            } else {
                next_state = READ_LITERAL;
            }
            // printf("READ_LITERAL \t lit_len:%d \n",lit_len);
        } else if (next_state == READ_OFFSET) {
            // printf("READ_OFFSET\n");
            offset.range(7, 0) = input_window.range(((input_index) + 1) * 8 - 1, (input_index)*8);
            offset.range(15, 8) = input_window.range(((input_index + 1) + 1) * 8 - 1, (input_index + 1) * 8);
            bool c0 = (token_match_len == 0xF);
            input_index += 2;
            match_len = token_match_len + 4; //+4 because of LZ4 standard
            offsetStream << offset;

            if (c0) {
                next_state = READ_MATCH_LEN;
            } else {
                next_state = READ_TOKEN;
                matchlenStream << match_len;
            }
            // printf("TokenMatchLen: %d\n",match_len);
        } else if (next_state == READ_MATCH_LEN) {
            // printf("READ_MATCH_LEN\n");
            ap_uint<8> token_value = input_window.range((input_index + 1) * 8 - 1, input_index * 8);
            input_index += 1;
            match_len += token_value;

            if (token_value == 0xFF) {
                next_state = READ_MATCH_LEN;
            } else {
                next_state = READ_TOKEN;
                matchlenStream << match_len;
            }
            // printf("MatchLen: %d next_state: %d\n",match_len,next_state);
        } else {
            // printf("INVALID\n");
            assert(0);
        }

        // If output written to buffer is more than or equals PARALLEL_BYTES
        //  then wite parallelBit data to outStream and shift the
        //  output buffer data
        if (output_index >= PARALLEL_BYTES) {
            outStreamValue.range(c_parallelBit - 1, 0) = output_window.range(c_parallelBit - 1, 0);
            output_window >>= PARALLEL_BYTES * 8;
            output_index = output_index - PARALLEL_BYTES;
            outFlag = true;
        } else if (output_index && (next_state == READ_OFFSET)) {
            outStreamValue.range(c_parallelBit - 1, 0) = output_window.range(c_parallelBit - 1, 0);
            output_index = 0;
            outFlag = true;
        } else {
            outFlag = false;
        }
        if (outFlag) {
            litStream << outStreamValue;
            out_written += PARALLEL_BYTES;
        }
    }

    // terminating last transaction
    matchlenStream << 0;
    offsetStream << 0;

    // signalling end of transaction
    litlenStream << 0;
    matchlenStream << 0;
    offsetStream << 0;
    // printf("\nInIdx: %d \t outIdx: %d \t Input_size: %d \t read_from_stream: %d  \t written_to_stream: %d \t
    // output_count: %d\n",input_index, output_index,input_size,readBytes, out_written, output_count);
}

template <int PARALLEL_BYTES, int HISTORY_SIZE>
void lz4DecompressEngine(hls::stream<ap_uint<PARALLEL_BYTES * 8> >& inStream,
                         hls::stream<ap_uint<PARALLEL_BYTES * 8> >& outStream,
                         hls::stream<bool>& outStreamEoS,
                         hls::stream<uint32_t>& outSizeStream,
                         const uint32_t _input_size) {
    typedef ap_uint<PARALLEL_BYTES * 8> uintV_t;
    typedef ap_uint<16> offset_dt;

    uint32_t input_size1 = _input_size;
    hls::stream<uint16_t> litlenStream("litlenStream");
    hls::stream<uintV_t> litStream("litStream");
    hls::stream<offset_dt> offsetStream("offsetStream");
    hls::stream<uint16_t> matchlenStream("matchlenStream");
#pragma HLS STREAM variable = litlenStream depth = 32
#pragma HLS STREAM variable = litStream depth = 32
#pragma HLS STREAM variable = offsetStream depth = 32
#pragma HLS STREAM variable = matchlenStream depth = 32

#pragma HLS RESOURCE variable = litlenStream core = FIFO_SRL
#pragma HLS RESOURCE variable = litStream core = FIFO_SRL
#pragma HLS RESOURCE variable = offsetStream core = FIFO_SRL
#pragma HLS RESOURCE variable = matchlenStream core = FIFO_SRL

#pragma HLS dataflow
    lz4MultiByteDecompress<PARALLEL_BYTES>(inStream, litlenStream, litStream, offsetStream, matchlenStream,
                                           input_size1);
    lzMultiByteDecoder<PARALLEL_BYTES, HISTORY_SIZE, uint16_t>(litlenStream, litStream, offsetStream, matchlenStream,
                                                               outStream, outStreamEoS, outSizeStream);
}

} // namespace compression
} // namespace xf
#endif // _XFCOMPRESSION_LZ4_DECOMPRESS_HPP_
