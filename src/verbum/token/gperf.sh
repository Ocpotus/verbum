#!/bin/bash

gperf -G -L ANSI-C -H token_hash_keyword -N token_is_keyword token.gperf > token.c
