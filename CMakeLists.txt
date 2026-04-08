cmake_minimum_required(VERSION 4.0)
project(untitled1 C)

set(CMAKE_C_STANDARD 17)

add_executable(untitled1 main.c
        logger.c
        logger.c
        generator.c
        time_utils.c
        worker.c
        config.h
        logger.h
        generator.h
        time_utils.h
        worker.h
        config.c)
