
/*
 * QuickJS libuv bindings
 * 
 * Copyright (c) 2019-present Saúl Ibarra Corretgé <s@saghul.net>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#ifndef QUV_VM_H
#define QUV_VM_H

#include <uv.h>
#include "../cutils.h"
#include "../quickjs-libuv.h"


typedef struct QUVRuntime QUVRuntime;

QUVRuntime *QUV_NewRuntime(void);
QUVRuntime *QUV_NewRuntime2(BOOL is_worker);
void QUV_FreeRuntime(QUVRuntime *qrt);
void QUV_SetupArgs(int argc, char **argv);
JSContext *QUV_GetJSContext(QUVRuntime *qrt);
QUVRuntime *QUV_GetRuntime(JSContext *ctx);
void QUV_Run(QUVRuntime *qrt);
void QUV_Stop(QUVRuntime *qrt);
uv_loop_t *QUV_GetLoop(QUVRuntime *qrt);

#endif
