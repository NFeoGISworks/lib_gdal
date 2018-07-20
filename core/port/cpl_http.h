/******************************************************************************
 * $Id$
 *
 * Project:  Common Portability Library
 * Purpose:  Function wrapper for libcurl HTTP access.
 * Author:   Frank Warmerdam, warmerdam@pobox.com
 *
 ******************************************************************************
 * Copyright (c) 2006, Frank Warmerdam
 * Copyright (c) 2009, Even Rouault <even dot rouault at mines-paris dot org>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 ****************************************************************************/

#ifndef CPL_HTTP_H_INCLUDED
#define CPL_HTTP_H_INCLUDED

#include "cpl_conv.h"
#include "cpl_string.h"
#include "cpl_progress.h"
#include "cpl_vsi.h"

#include <vector>

/**
 * \file cpl_http.h
 *
 * Interface for downloading HTTP, FTP documents
 */

/*! @cond Doxygen_Suppress */
#define CPL_HTTP_MAX_RETRY      0
#define CPL_HTTP_RETRY_DELAY    30.0
/*! @endcond */

CPL_C_START

/*! Describe a part of a multipart message */
typedef struct {
    /*! NULL terminated array of headers */ char **papszHeaders;

    /*! Buffer with data of the part     */ GByte *pabyData;
    /*! Buffer length                    */ int    nDataLen;
} CPLMimePart;

/*! Describe the result of a CPLHTTPFetch() call */
typedef struct {
    /*! cURL error code : 0=success, non-zero if request failed */
    int     nStatus;

    /*! Content-Type of the response */
    char    *pszContentType;

    /*! Error message from curl, or NULL */
    char    *pszErrBuf;

    /*! Length of the pabyData buffer */
    int     nDataLen;
    /*! Allocated size of the pabyData buffer */
    int     nDataAlloc;

    /*! Buffer with downloaded data */
    GByte   *pabyData;

    /*! Headers returned */
    char    **papszHeaders;

    /*! Number of parts in a multipart message */
    int     nMimePartCount;

    /*! Array of parts (resolved by CPLHTTPParseMultipartMime()) */
    CPLMimePart *pasMimePart;

} CPLHTTPResult;

/*! @cond Doxygen_Suppress */
typedef size_t (*CPLHTTPFetchWriteFunc)(void *pBuffer, size_t nSize, size_t nMemb, void *pWriteArg);
/*! @endcond */

int CPL_DLL   CPLHTTPEnabled( void );
CPLHTTPResult CPL_DLL *CPLHTTPFetch( const char *pszURL, CSLConstList papszOptions);
CPLHTTPResult CPL_DLL *CPLHTTPFetchEx( const char *pszURL,CSLConstList papszOptions,
                                       GDALProgressFunc pfnProgress,
                                       void *pProgressArg,
                                       CPLHTTPFetchWriteFunc pfnWrite,
                                       void *pWriteArg);
CPLHTTPResult CPL_DLL **CPLHTTPMultiFetch( const char * const * papszURL,
                                           int nURLCount,
                                           int nMaxSimultaneous,
                                           CSLConstList papszOptions);

void CPL_DLL  CPLHTTPCleanup( void );
void CPL_DLL  CPLHTTPDestroyResult( CPLHTTPResult *psResult );
void CPL_DLL  CPLHTTPDestroyMultiResult( CPLHTTPResult **papsResults, int nCount );
int  CPL_DLL  CPLHTTPParseMultipartMime( CPLHTTPResult *psResult );

/* -------------------------------------------------------------------- */
/*      The following is related to OAuth2 authorization around         */
/*      google services like fusion tables, and potentially others      */
/*      in the future.  Code in cpl_google_oauth2.cpp.                  */
/*                                                                      */
/*      These services are built on CPL HTTP services.                  */
/* -------------------------------------------------------------------- */

char CPL_DLL *GOA2GetAuthorizationURL( const char *pszScope );
char CPL_DLL *GOA2GetRefreshToken( const char *pszAuthToken,
                                   const char *pszScope );
char CPL_DLL *GOA2GetAccessToken( const char *pszRefreshToken,
                                  const char *pszScope );

char  CPL_DLL **GOA2GetAccessTokenFromServiceAccount(
                                        const char* pszPrivateKey,
                                        const char* pszClientEmail,
                                        const char* pszScope,
                                        CSLConstList papszAdditionalClaims,
                                        CSLConstList papszOptions);

char CPL_DLL **GOA2GetAccessTokenFromCloudEngineVM( CSLConstList papszOptions );

/* HTTP Auth event triggers */
typedef enum  {
    HTTPAUTH_UPDATE = 1, /*< Auth properties updated */
    HTTPAUTH_EXPIRED     /*< Auth properties is expired and cannot be used any more. Need user interaction */
} CPLHTTPAuthChangeCode;

/**
 * @brief Prototype of function, which executed when changes accured.
 * @param pszUrl The URL of server which token changed
 * @param eOperation Operation which trigger notification.
 */
typedef void (*HTTPAuthNotifyFunc)(const char *pszUrl, CPLHTTPAuthChangeCode eOperation);

int CPL_DLL CPLHTTPAuthAdd(const char *pszUrl, char **papszOptions,
                            HTTPAuthNotifyFunc func);
void CPL_DLL CPLHTTPAuthDelete(const char *pszUrl);
char ** CPL_DLL CPLHTTPAuthProperties(const char *pszUrl);
const char *CPL_DLL CPLHTTPAuthHeader(const char *pszUrl);

CPL_C_END

#if defined(__cplusplus) && !defined(CPL_SUPRESS_CPLUSPLUS)
/*! @cond Doxygen_Suppress */
// Not sure if this belong here, used in cpl_http.cpp, cpl_vsil_curl.cpp and frmts/wms/gdalhttp.cpp
void* CPLHTTPSetOptions(void *pcurl, const char * const* papszOptions);
char** CPLHTTPGetOptionsFromEnv();
double CPLHTTPGetNewRetryDelay(int response_code, double dfOldDelay);
void* CPLHTTPIgnoreSigPipe();
void CPLHTTPRestoreSigPipeHandler(void* old_handler);
bool CPLMultiPerformWait(void* hCurlMultiHandle, int& repeats);
/*! @endcond */

bool CPLIsMachinePotentiallyGCEInstance();
bool CPLIsMachineForSureGCEInstance();

/** Manager of Google OAuth2 authentication.
 *
 * This class handles different authentication methods and handles renewal
 * of access token.
 *
 * @since GDAL 2.3
 */
class GOA2Manager
{
    public:

        GOA2Manager();

        /** Authentication method */
        typedef enum
        {
            NONE,
            GCE,
            ACCESS_TOKEN_FROM_REFRESH,
            SERVICE_ACCOUNT
        } AuthMethod;

        bool SetAuthFromGCE( CSLConstList papszOptions );
        bool SetAuthFromRefreshToken( const char* pszRefreshToken,
                                      const char* pszClientId,
                                      const char* pszClientSecret,
                                      CSLConstList papszOptions );
        bool SetAuthFromServiceAccount(const char* pszPrivateKey,
                                       const char* pszClientEmail,
                                       const char* pszScope,
                                       CSLConstList papszAdditionalClaims,
                                       CSLConstList papszOptions );

        /** Returns the authentication method. */
        AuthMethod GetAuthMethod() const { return m_eMethod; }

        const char* GetBearer() const;

        /** Returns private key for SERVICE_ACCOUNT method */
        const CPLString& GetPrivateKey() const { return m_osPrivateKey; }

        /** Returns client email for SERVICE_ACCOUNT method */
        const CPLString& GetClientEmail() const { return m_osClientEmail; }

    private:

        mutable CPLString       m_osCurrentBearer;
        mutable time_t          m_nExpirationTime;
        AuthMethod      m_eMethod;

        // for ACCESS_TOKEN_FROM_REFRESH
        CPLString       m_osClientId;
        CPLString       m_osClientSecret;
        CPLString       m_osRefreshToken;

        // for SERVICE_ACCOUNT
        CPLString       m_osPrivateKey;
        CPLString       m_osClientEmail;
        CPLString       m_osScope;
        CPLStringList   m_aosAdditionalClaims;

        CPLStringList   m_aosOptions;
};

/**
 * @brief The IHTTPAuth class is base class for HTTP Authorization headers
 */
class IHTTPAuth {
public:
    virtual ~IHTTPAuth() {}
    virtual const char *GetUrl() const = 0;
    virtual const char *GetHeader() = 0;
    virtual char **GetProperties() const = 0;
};

/**
 * @brief The CPLHTTPAuthBearer class The oAuth2 bearer update token class
 */
class CPLHTTPAuthBearer : public IHTTPAuth {

public:
    explicit CPLHTTPAuthBearer(const CPLString& soUrl, const CPLString& soClientId,
                               const CPLString& soTokenServer,
                               const CPLString& soAccessToken,
                               const CPLString& soUpdateToken, int nExpiresIn,
                               HTTPAuthNotifyFunc function,
                               const CPLString& soConnTimeout,
                               const CPLString& soTimeout,
                               const CPLString& soMaxRetry,
                               const CPLString& soRetryDelay);
    virtual ~CPLHTTPAuthBearer() {}
    virtual const char *GetUrl() const CPL_OVERRIDE { return m_soUrl; }
    virtual const char *GetHeader() CPL_OVERRIDE;
    virtual char **GetProperties() const CPL_OVERRIDE;

private:
    CPLString m_soUrl;
    CPLString m_soClientId;
    CPLString m_soAccessToken;
    CPLString m_soUpdateToken;
    CPLString m_soTokenServer;
    int m_nExpiresIn;
    HTTPAuthNotifyFunc m_NotifyFunction;
    CPLString m_soConnTimeout;
    CPLString m_soTimeout;
    CPLString m_soMaxRetry;
    CPLString m_soRetryDelay;
    time_t m_nLastCheck;
};

/**
 * @brief The CPLHTTPAuthStore class Storage for authorisation options
 */
class CPLHTTPAuthStore
{
public:
    static bool Add(const char *pszUrl, char **papszOptions,
                    HTTPAuthNotifyFunc func = nullptr);
    static CPLHTTPAuthStore& instance();

public:
    void Add(IHTTPAuth *poAuth);
    void Delete(const char *pszUrl);
    const char* GetAuthHeader(const char *pszUrl);
    char** GetProperties(const char *pszUrl) const;

private:
    CPLHTTPAuthStore() {}
    ~CPLHTTPAuthStore() {}
    CPLHTTPAuthStore(CPLHTTPAuthStore const&) {}
    CPLHTTPAuthStore& operator= (CPLHTTPAuthStore const&) { return *this; }

private:
    std::vector<IHTTPAuth*> m_poAuths;
};

#endif // __cplusplus

#endif /* ndef CPL_HTTP_H_INCLUDED */
