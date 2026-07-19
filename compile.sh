#!/bin/bash -xe

gcc -W{all,extra,conversion,sign-conversion} -std=c99 -O2 -g trash.c -o trash
