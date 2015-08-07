/* The CGI_C library, by Thomas Boutell, version 2.01. CGI_C is intended
	to be a high-quality API to simplify CGI programming tasks. */

/* Make sure this is only included once. */

#ifndef CGI_C
#define CGI_C 1

/* Bring in standard I/O since some of the functions refer to
	types defined by it, such as FILE *. */

#include <stdio.h>

/* The various CGI environment variables. Instead of using getenv(),
	the programmer should refer to these, which are always
	valid null-terminated strings (they may be empty, but they 
	will never be null). If these variables are used instead
	of calling getenv(), then it will be possible to save
	and restore CGI environments, which is highly convenient
	for debugging. */

extern char *cgiServerSoftware;
extern char *cgiServerName;
extern char *cgiGatewayInterface;
extern char *cgiServerProtocol;
extern char *cgiServerPort;
extern char *cgiRequestMethod;
extern char *cgiPathInfo;
extern char *cgiPathTranslated;
extern char *cgiScriptName;
extern char *cgiQueryString;
extern char *cgiRemoteHost;
extern char *cgiRemoteAddr;
extern char *cgiAuthType;
extern char *cgiRemoteUser;
extern char *cgiRemoteIdent;
extern char *cgiContentType;
extern char *cgiAccept;
extern char *cgiUserAgent;
extern char *cgiReferrer;

/* Cookies as sent to the server. You can also get them
	individually, or as a string array; see the documentation. */
extern char *cgiCookie;

/* A macro providing the same incorrect spelling that is
	found in the HTTP/CGI specifications */
#define cgiReferer cgiReferrer

/* The number of bytes of data received.
	Note that if the submission is a form submission
	the library will read and parse all the information
	directly from cgiIn; the programmer need not do so. */

extern int cgiContentLength;

/* Pointer to CGI output. The cgiHeader functions should be used
	first to output the mime headers; the output HTML
	page, GIF image or other web document should then be written
	to cgiOut by the programmer. In the standard CGIC library,
	cgiOut is always equivalent to stdout. */

extern FILE *cgiOut;

/* Pointer to CGI input. The programmer does not read from this.
	We have continued to export it for backwards compatibility
	so that cgic 1.x applications link properly. */

extern FILE *cgiIn;

/* Possible return codes from the cgiForm family of functions (see below). */

typedef enum {
	cgiFormSuccess,
	cgiFormTruncated,
	cgiFormBadType,
	cgiFormEmpty,
	cgiFormNotFound,
	cgiFormConstrained,
	cgiFormNoSuchChoice,
	cgiFormMemory,
	cgiFormNoFileName,
	cgiFormNoContentType,
	cgiFormNotAFile,
	cgiFormOpenFailed,
	cgiFormIO,
	cgiFormEOF
} cgiFormResultType;

/* These functions are used to retrieve form data. See
	cgic.html for documentation. */

extern cgiFormResultType cgiFormString(
	char *name, char *result, int max);

extern cgiFormResultType cgiFormStringNoNewlines(
	char *name, char *result, int max);


extern cgiFormResultType cgiFormStringSpaceNeeded(
	char *name, int *length);


extern cgiFormResultType cgiFormStringMultiple(
	char *name, char ***ptrToStringArray);

extern void cgiStringArrayFree(char **stringArray);

extern cgiFormResultType cgiFormInteger(
	char *name, int *result, int defaultV);

extern cgiFormResultType cgiFormIntegerBounded(
	char *name, int *result, int min, int max, int defaultV);

extern cgiFormResultType cgiFormDouble(
	char *name, double *result, double defaultV);

extern cgiFormResultType cgiFormDoubleBounded(
	char *name, double *result, double min, double max, double defaultV);

extern cgiFormResultType cgiFormSelectSingle(
	char *name, char **choicesText, int choicesTotal, 
	int *result, int defaultV);	


extern cgiFormResultType cgiFormSelectMultiple(
	char *name, char **choicesText, int choicesTotal, 
	int *result, int *invalid);

/* Just an alias; users have asked for this */
#define cgiFormSubmitClicked cgiFormCheckboxSingle

extern cgiFormResultType cgiFormCheckboxSingle(
	char *name);

extern cgiFormResultType cgiFormCheckboxMultiple(
	char *name, char **valuesText, int valuesTotal, 
	int *result, int *invalid);

extern cgiFormResultType cgiFormRadio(
	char *name, char **valuesText, int valuesTotal, 
	int *result, int defaultV);	

/* The paths returned by this function are the original names of files
	as reported by the uploading web browser and shoult NOT be
	blindly assumed to be "safe" names for server-side use! */
extern cgiFormResultType cgiFormFileName(
	char *name, char *result, int max);

/* The content type of the uploaded file, as reported by the browser.
	It should NOT be assumed that browsers will never falsify
	such information. */
extern cgiFormResultType cgiFormFileContentType(
	char *name, char *result, int max);

extern cgiFormResultType cgiFormFileSize(
	char *name, int *sizeP);

typedef struct cgiFileStruct *cgiFilePtr;

extern cgiFormResultType cgiFormFileOpen(
	char *name, cgiFilePtr *cfpp);

extern cgiFormResultType cgiFormFileRead(
	cgiFilePtr cfp, char *buffer, int bufferSize, int *gotP);

extern cgiFormResultType cgiFormFileClose(
	cgiFilePtr cfp);

extern cgiFormResultType cgiCookieString(
	char *name, char *result, int max);

extern cgiFormResultType cgiCookieInteger(
	char *name, int *result, int defaultV);

cgiFormResultType cgiCookies(
	char ***ptrToStringArray);

/* path can be null or empty in which case a path of / (entire site) is set. 
	domain can be a single web site; if it is an entire domain, such as
	'boutell.com', it should begin with a dot: '.boutell.com' */
extern void cgiHeaderCookieSetString(char *name, char *value, 
	int secondsToLive, char *path, char *domain);
extern void cgiHeaderCookieSetInteger(char *name, int value,
	int secondsToLive, char *path, char *domain);
extern void cgiHeaderLocation(char *redirectUrl);
extern void cgiHeaderStatus(int status, char *statusMessage);
extern void cgiHeaderContentType(char *mimeType);

typedef enum {
	cgiEnvironmentIO,
	cgiEnvironmentMemory,
	cgiEnvironmentSuccess,
	cgiEnvironmentWrongVersion
} cgiEnvironmentResultType;

extern cgiEnvironmentResultType cgiWriteEnvironment(char *filename);
extern cgiEnvironmentResultType cgiReadEnvironment(char *filename);

extern int cgiMain();

extern cgiFormResultType cgiFormEntries(
	char ***ptrToStringArray);

/* Output string with the <, &, and > characters HTML-escaped. 
	's' is null-terminated. Returns cgiFormIO in the event
	of error, cgiFormSuccess otherwise. */
cgiFormResultType cgiHtmlEscape(const char *s);

/* Output data with the <, &, and > characters HTML-escaped. 
	'data' is not null-terminated; 'len' is the number of
	bytes in 'data'. Returns cgiFormIO in the event
	of error, cgiFormSuccess otherwise. */
cgiFormResultType cgiHtmlEscapeData(const char *data, int len);

/* Output string with the " character HTML-escaped, and no
	other characters escaped. This is useful when outputting
	the contents of a tag attribute such as 'href' or 'src'.
	's' is null-terminated. Returns cgiFormIO in the event
	of error, cgiFormSuccess otherwise. */
cgiFormResultType cgiValueEscape(const char *s);

/* Output data with the " character HTML-escaped, and no
	other characters escaped. This is useful when outputting
	the contents of a tag attribute such as 'href' or 'src'.
	'data' is not null-terminated; 'len' is the number of
	bytes in 'data'. Returns cgiFormIO in the event
	of error, cgiFormSuccess otherwise. */
cgiFormResultType cgiValueEscapeData(const char *data, int len);

#endif /* CGI_C */

