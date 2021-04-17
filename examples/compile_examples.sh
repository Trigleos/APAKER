#!/bin/bash
gcc -no-pie example.c -o example_nopie
gcc example.c -o example_pie
