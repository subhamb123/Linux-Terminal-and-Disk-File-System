cmake_minimum_required(VERSION 3.23)
project(Final)

set(CMAKE_CXX_STANDARD 14)

add_executable(Final main.c alloc_dalloc.c cd_ls_pwd.c link_unlink.c mkdir_creat.c rmdir.c symlink.c util.c open_close_lseek.c read_cat.c write_cp.c)
