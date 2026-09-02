/*
 * SPDX-License-Identifier: Apache-2.0
 * Copyright 2019-2021 Western Digital Corporation or its affiliates.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
*/

/*
 Linker script
*/

OUTPUT_ARCH( "riscv" )

ENTRY( _start )


MEMORY
{
  
  RAM  (wxa!ri) : ORIGIN = 0x00000000, LENGTH = 128M
  ICCM (wxa!ri) : ORIGIN = 0xEE000000, LENGTH = 64K
  DCCM (wxa!ri) : ORIGIN = 0xF0040000, LENGTH = 32K
}




/*----------------------------------------------------------------------*/
/* Sections                            */
/*----------------------------------------------------------------------*/

SECTIONS
{
  __stack_size = 4K;
  __heap_size = 8K;

  .text.init :
  {
    *(.text.init)
    . = ALIGN(8);
  } > ICCM

  .text :
  {
    *(.text.unlikely .text.unlikely.*)
    *(.text.startup .text.startup.*)
    *(.text .text.*)
    *(.gnu.linkonce.t.*)
    . = ALIGN(4);
  } > ICCM


  .rodata :
  {
    *(.rdata)
    *(.rodata .rodata.*)
    *(.gnu.linkonce.r.*)
    . = ALIGN(4);
  } > DCCM

  .lalign :
  {
    . = ALIGN(4);
    PROVIDE( _data_lma = . );
  } > DCCM

  .dalign :
  {
    . = ALIGN(4);
    PROVIDE( _data = . );
  } > DCCM

  .data :
  {
    *(.ext_interrupt_vector)
    *(.data .data.*)
    *(.gnu.linkonce.d.*)
    . += 10; /* fix for linker false error message */
    . = ALIGN(8);
  } > DCCM 

  .sdata :
  {
    . = ALIGN(8);
    __global_pointer$ = . + 0x800;
    *(.sdata .sdata.*)
    *(.gnu.linkonce.s.*)
    . = ALIGN(8);
    *(.srodata .srodata.*)
   . = ALIGN(8);
  } >DCCM 

  . = ALIGN(4);
  PROVIDE( _edata = . );
  PROVIDE( edata = . );

  PROVIDE( _fbss = . );
  PROVIDE( __bss_start = . );

  .bss :
  {
    *(.sbss .sbss.* .gnu.linkonce.sb.*)
    *(.scommon)
    *(.bss .bss.*)
    . = ALIGN(8);
  } >DCCM
  PROVIDE( __bss_end = . );
  _end = .;
  end = .;

  .heap :
  {
   . = ALIGN(8);
    _heap_start = .;
     . = . + __heap_size;
     _heap_end = .;
   . = ALIGN(8);
  } > DCCM

  .stack :
  {
   . = ALIGN(8);
  	_stack_start = . ;
    . = . + __stack_size;
    . = ALIGN(8);
    _stack_end = . ;
    _sp = .;
  } > DCCM


}
