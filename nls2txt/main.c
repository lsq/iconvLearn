/*
 * https://doxygen.reactos.org/d5/df2/modules_2rosapps_2applications_2devutils_2nls2txt_2main_8c.html
 * PROJECT:     ReactOS NLS to TXT Converter
 * LICENSE:     GNU General Public License Version 2.0 or any later version
 * FILE:        devutils/nls2txt/main.c
 * COPYRIGHT:   Copyright 2016 Dmitry Chapyshev <dmitry@reactos.org>
 */
 
#include "precomp.h"
 
INT wmain(INT argc, WCHAR* argv[])
{
    if (argc != 3)
        return 1;
 
    if (!BestFit_FromNLS(argv[1], argv[2]))
        return 1;
 
    return 0;
}
