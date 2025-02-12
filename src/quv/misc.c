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

#include <unistd.h>

#include "../cutils.h"
#include "error.h"
#include "misc.h"
#include "utils.h"


static JSValue quv_hrtime(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    return JS_NewBigUint64(ctx, uv_hrtime());
}

static JSValue quv_gettimeofday(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    uv_timeval64_t tv;
    int r = uv_gettimeofday(&tv);
    if (r != 0)
        return quv_throw_errno(ctx, r);
    return JS_NewInt64(ctx, tv.tv_sec * 1000 + (tv.tv_usec / 1000));
}

static JSValue quv_uname(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    JSValue obj;
    int r;
    uv_utsname_t utsname;

    r = uv_os_uname(&utsname);
    if (r != 0)
        return quv_throw_errno(ctx, r);

    obj = JS_NewObjectProto(ctx, JS_NULL);
    JS_DefinePropertyValueStr(ctx, obj, "sysname", JS_NewString(ctx, utsname.sysname), JS_PROP_C_W_E);
    JS_DefinePropertyValueStr(ctx, obj, "release", JS_NewString(ctx, utsname.release), JS_PROP_C_W_E);
    JS_DefinePropertyValueStr(ctx, obj, "version", JS_NewString(ctx, utsname.version), JS_PROP_C_W_E);
    JS_DefinePropertyValueStr(ctx, obj, "machine", JS_NewString(ctx, utsname.machine), JS_PROP_C_W_E);

    return obj;
}

static JSValue quv_isatty(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    int fd, type;
    if (JS_ToInt32(ctx, &fd, argv[0]))
        return JS_EXCEPTION;

    type = uv_guess_handle(fd);
    return JS_NewBool(ctx, type == UV_TTY);
}

static JSValue quv_environ(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    uv_env_item_t *env;
    int envcount, r;

    r = uv_os_environ(&env, &envcount);
    if (r != 0)
        return quv_throw_errno(ctx, r);

    JSValue obj = JS_NewObjectProto(ctx, JS_NULL);

    for (int i = 0; i < envcount; i++) {
        JS_DefinePropertyValueStr(ctx, obj, env[i].name, JS_NewString(ctx, env[i].value), JS_PROP_C_W_E);
    }

    uv_os_free_environ(env, envcount);

    return obj;
}

static JSValue quv_getenv(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    const char *name = JS_ToCString(ctx, argv[0]);
    if (!name)
        return JS_EXCEPTION;

    char buf[1024];
    size_t size = sizeof(buf);
    char *dbuf = buf;
    int r;

    r = uv_os_getenv(name, dbuf, &size);
    if (r != 0) {
        if (r != UV_ENOBUFS)
            return quv_throw_errno(ctx, r);
        dbuf = js_malloc(ctx, size);
        if (!dbuf)
            return JS_EXCEPTION;
        r = uv_os_getenv(name, dbuf, &size);
        if (r != 0) {
            js_free(ctx, dbuf);
            return quv_throw_errno(ctx, r);
        }
    }

    JSValue ret = JS_NewStringLen(ctx, dbuf, size);

    if (dbuf != buf)
        js_free(ctx, dbuf);

    return ret;
}

static JSValue quv_setenv(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    const char *name = JS_ToCString(ctx, argv[0]);
    if (!name)
        return JS_EXCEPTION;

    const char *value = JS_ToCString(ctx, argv[1]);
    if (!value)
        return JS_EXCEPTION;

    int r = uv_os_setenv(name, value);
    if (r != 0)
        return quv_throw_errno(ctx, r);

    return JS_UNDEFINED;
}

static JSValue quv_unsetenv(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    const char *name = JS_ToCString(ctx, argv[0]);
    if (!name)
        return JS_EXCEPTION;

    int r = uv_os_unsetenv(name);
    if (r != 0)
        return quv_throw_errno(ctx, r);

    return JS_UNDEFINED;
}

static JSValue quv_cwd(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
    char buf[1024];
    size_t size = sizeof(buf);
    char *dbuf = buf;
    int r;

    r = uv_cwd(dbuf, &size);
    if (r != 0) {
        if (r != UV_ENOBUFS)
            return quv_throw_errno(ctx, r);
        dbuf = js_malloc(ctx, size);
        if (!dbuf)
            return JS_EXCEPTION;
        r = uv_cwd(dbuf, &size);
        if (r != 0) {
            js_free(ctx, dbuf);
            return quv_throw_errno(ctx, r);
        }
    }

    JSValue ret = JS_NewStringLen(ctx, dbuf, size);

    if (dbuf != buf)
        js_free(ctx, dbuf);

    return ret;
}

static JSValue quv_homedir(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
    char buf[1024];
    size_t size = sizeof(buf);
    char *dbuf = buf;
    int r;

    r = uv_os_homedir(dbuf, &size);
    if (r != 0) {
        if (r != UV_ENOBUFS)
            return quv_throw_errno(ctx, r);
        dbuf = js_malloc(ctx, size);
        if (!dbuf)
            return JS_EXCEPTION;
        r = uv_os_homedir(dbuf, &size);
        if (r != 0) {
            js_free(ctx, dbuf);
            return quv_throw_errno(ctx, r);
        }
    }

    JSValue ret = JS_NewStringLen(ctx, dbuf, size);

    if (dbuf != buf)
        js_free(ctx, dbuf);

    return ret;
}

static JSValue quv_tmpdir(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
    char buf[1024];
    size_t size = sizeof(buf);
    char *dbuf = buf;
    int r;

    r = uv_os_tmpdir(dbuf, &size);
    if (r != 0) {
        if (r != UV_ENOBUFS)
            return quv_throw_errno(ctx, r);
        dbuf = js_malloc(ctx, size);
        if (!dbuf)
            return JS_EXCEPTION;
        r = uv_os_tmpdir(dbuf, &size);
        if (r != 0) {
            js_free(ctx, dbuf);
            return quv_throw_errno(ctx, r);
        }
    }

    JSValue ret = JS_NewStringLen(ctx, dbuf, size);

    if (dbuf != buf)
        js_free(ctx, dbuf);

    return ret;
}

static JSValue quv_exepath(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
    char buf[1024];
    size_t size = sizeof(buf);
    char *dbuf = buf;
    int r;

    r = uv_exepath(dbuf, &size);
    if (r != 0) {
        if (r != UV_ENOBUFS)
            return quv_throw_errno(ctx, r);
        dbuf = js_malloc(ctx, size);
        if (!dbuf)
            return JS_EXCEPTION;
        r = uv_exepath(dbuf, &size);
        if (r != 0) {
            js_free(ctx, dbuf);
            return quv_throw_errno(ctx, r);
        }
    }

    JSValue ret = JS_NewStringLen(ctx, dbuf, size);

    if (dbuf != buf)
        js_free(ctx, dbuf);

    return ret;
}

static const JSCFunctionListEntry quv_misc_funcs[] = {
    QUV_CONST(AF_INET),
    QUV_CONST(AF_INET6),
    QUV_CONST(AF_UNSPEC),
    QUV_CONST(STDIN_FILENO),
    QUV_CONST(STDOUT_FILENO),
    QUV_CONST(STDERR_FILENO),
    QUV_CONST(UV_TCP_IPV6ONLY),
    QUV_CONST(UV_TTY_MODE_NORMAL),
    QUV_CONST(UV_TTY_MODE_RAW),
    QUV_CONST(UV_TTY_MODE_IO),
    QUV_CONST(UV_UDP_IPV6ONLY),
    QUV_CONST(UV_UDP_PARTIAL),
    QUV_CONST(UV_UDP_REUSEADDR),
    JS_CFUNC_DEF("hrtime", 0, quv_hrtime ),
    JS_CFUNC_DEF("gettimeofday", 0, quv_gettimeofday ),
    JS_CFUNC_DEF("uname", 0, quv_uname ),
    JS_CFUNC_DEF("isatty", 1, quv_isatty ),
    JS_CFUNC_DEF("environ", 0, quv_environ ),
    JS_CFUNC_DEF("getenv", 0, quv_getenv ),
    JS_CFUNC_DEF("setenv", 2, quv_setenv ),
    JS_CFUNC_DEF("unsetenv", 1, quv_unsetenv ),
    JS_CFUNC_DEF("cwd", 0, quv_cwd ),
    JS_CFUNC_DEF("homedir", 0, quv_homedir ),
    JS_CFUNC_DEF("tmpdir", 0, quv_tmpdir ),
    JS_CFUNC_DEF("exepath", 0, quv_exepath ),
};

void quv_mod_misc_init(JSContext *ctx, JSModuleDef *m) {
    JS_SetModuleExportList(ctx, m, quv_misc_funcs, countof(quv_misc_funcs));
}

void quv_mod_misc_export(JSContext *ctx, JSModuleDef *m) {
    JS_AddModuleExportList(ctx, m, quv_misc_funcs, countof(quv_misc_funcs));
}
