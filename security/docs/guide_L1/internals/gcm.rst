.. 
   Copyright 2019 Xilinx, Inc.
  
   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at
  
       http://www.apache.org/licenses/LICENSE-2.0
  
   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.

********
GCM Mode
********

.. toctree::
   :maxdepth: 1

Overview
========

The Galois/Counter (GCM) mode is a typical block cipher mode of operation using block cipher algorithm.
In this version, we provide Advanced Encryption Standard (AES) processing ability,
the cipherkey length for AES should be 128/192/256 bits.
Our working mode works on units of a fixed size (128 bits for 1 block), but text in the real world has a variety of lengths.
Therefore, the last block of the text must be padded to 128 bits before encryption or decryption.

.. Caution::
    Applicable conditions:
    1. In this version, we provide the simplest way as the IEEE standard mentioned to generate initial counter block (ICB), so the length of the initialization vector (IV) should be 96 bits.
    2. For calculating the tag, the additional authenticated data (AAD) is restricted to 128 bits (all zeros) in this design.
    That is to say, you cannot give an AAD (we don't have one as in much applications), you should set the AAD to zero.

Implementation on FPGA
======================

We support GCM mode including both encryption and decryption parts in this implementation.

.. ATTENTION::
    The bit-width of the interfaces we provide is shown as follows:
    ============ =========== ============ =========== ==== ===== =====
                  plaintext   ciphertext   cipherkey   IV   AAD   tag
    ============ =========== ============ =========== ==== ===== =====
     GCM-AES128      128         128          128      96   128   128
    ============ =========== ============ =========== ==== ===== =====
     GCM-AES192      128         128          192      96   128   128
    ============ =========== ============ =========== ==== ===== =====
     GCM-AES256      128         128          256      96   128   128
    ============ =========== ============ =========== ==== ===== =====

To give a similar interface as the encryption part, we provide the tag value instead of a FAIL flag in the decryption part.
Thus, you need to compare it with the tag which is generated by the GCM encryption part to judge whether the data is authentic or not.
The algorithm flow chart of encryption part of GCM mode is shown as follow:

.. image:: /images/GCM_encryption.png
   :alt: algorithm flow chart of GCM_encryption
   :width: 100%
   :align: center

As we can see from the chart, the GCM encryption part can be divided into two individual parts: The Counter Mode (CTR) and The Galois Message Authentication Code (GMAC).
GCM is used to encrypt the plaintext to ciphertext, and GMAC is used to generate the data tag. 
The algorithm flow chart of decryption part of GCM mode is shown as follow:

.. image:: /images/GCM_decryption.png
   :alt: algorithm flow chart of GCM_decryption
   :width: 100%
   :align: center

The decryption part is very similar with the encryption part of GCM mode.
The only difference is that we decrypt the ciphertext to plaintext in the decryption part.
In decryption part of GCM mode, we don’t provide a FAIL flag to indicate whether the data is authentic or not.
You should compare the tag which the decryption part gives with the tag from GCM encryption part to judge the authenticity of the data.
If the data is authentic, then the tags should be equal.

In our implementation, the encryption part of GCM mode has three modules, which are ciphertext_gen, dup_strm, and tag_gen.
As the three modules can work independently, they are designed into parallel dataflow processes, and connected by streams (FIFO's).
Thus, the AES_GCM_encryption which is the wrapper of the three modules, can achieve an initiation interval (II) = 1.
The decryption part can be deduced in the same way, so its II = 1.

Profiling
=========

GCM-AES128 encryption
---------------------

====== ====== ====== ===== ====== ===== ====== ========
 CLB    LUT     FF    DSP   BRAM   SRL   URAM   CP(ns)
====== ====== ====== ===== ====== ===== ====== ========

====== ====== ====== ===== ====== ===== ====== ========

GCM-AES128 decryption
---------------------

====== ====== ====== ===== ====== ====== ====== ========
 CLB    LUT     FF    DSP   BRAM   SRL    URAM   CP(ns)
====== ====== ====== ===== ====== ====== ====== ========

====== ====== ====== ===== ====== ====== ====== ========

GCM-AES192 encryption
---------------------

====== ====== ====== ===== ====== ===== ====== ========
 CLB    LUT     FF    DSP   BRAM   SRL   URAM   CP(ns)
====== ====== ====== ===== ====== ===== ====== ========

====== ====== ====== ===== ====== ===== ====== ========

GCM-AES192 decryption
---------------------

====== ====== ====== ===== ====== ====== ====== ========
 CLB    LUT     FF    DSP   BRAM   SRL    URAM   CP(ns)
====== ====== ====== ===== ====== ====== ====== ========

====== ====== ====== ===== ====== ====== ====== ========

GCM-AES256 encryption
---------------------

====== ====== ====== ===== ====== ===== ====== ========
 CLB    LUT     FF    DSP   BRAM   SRL   URAM   CP(ns)
====== ====== ====== ===== ====== ===== ====== ========

====== ====== ====== ===== ====== ===== ====== ========

GCM-AES256 decryption
---------------------

====== ====== ====== ===== ====== ====== ====== ========
 CLB    LUT     FF    DSP   BRAM   SRL    URAM   CP(ns)
====== ====== ====== ===== ====== ====== ====== ========

====== ====== ====== ===== ====== ====== ====== ========
