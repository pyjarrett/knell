#ifndef CN_H
#define CN_H

#ifdef __cplusplus
extern "C" {
#endif

/*
 * This is the "standard" Calendon header which most files should include to get
 * the basic types and base functionality and macros used in Calendon code.
 *
 * The size of this prelude-style header should be reduced to minimum practical
 * to prevent introducing excessive elements throughout the engine and bloating
 * compile times.
 *
 * This header should always be included first since it is likely to be
 * converted into a precompiled header.
 */
#include <inttypes.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

/*
 * Features switches change behavior at compile time to improve debugging,
 * or expose additional behavior:
 * 
 * - `CN_DEBUG`: Debug builds.  Gate features which should never affect end users.
 * - `CN_TESTING`: When building the Calendon library for testing.  Exposes
 *   behavior used for testing.
 */

/*
 * Calendon aims to limit the number of exported functions for simplicity and
 * provides symbol export/import markers for engine library functions.
 *
 * - Use `CN_API` for engine library functions (within calendon).
 * - Use `CN_GAME_API` for game library functions exposed to Calendon.
 * - Use `CN_TEST_API` for exposing functions for testing.
 *
 * Windows makes symbols within DLLs invisible by default, so use `CN_TEST_API`
 * to expose functions for testing purposes.  This limits the possibility of
 * client creating dependencies on features only exposed for testing.
 *
 * Windows:
 * - dllimport - describe that we're going to find this when we link
 * - dllexport - we're providing this function for others to use
 *
 * Linux:
 * - Overrides the hidden visibility set by default by our build.
 * - __attribute__((visibility("default")) the symbol should be made visible
 */
#ifdef _WIN32
	#if CN_LIBRARY
		#define CN_API __declspec(dllexport)
		#if CN_TESTING
			#define CN_TEST_API __declspec(dllexport)
		#else
			#define CN_TEST_API
		#endif
	#else
		#define CN_API __declspec(dllimport)
		#if CN_TESTING
			#define CN_TEST_API __declspec(dllimport)
		#else
			#define CN_TEST_API
		#endif
	#endif
	#define CN_GAME_API __declspec(dllexport)
#else
	#if CN_LIBRARY
		#define CN_API __attribute__((visibility("default")))
	#else
		#define CN_API __attribute__((visibility("default")))
	#endif
	#if CN_TESTING
		#define CN_TEST_API __attribute__((visibility("default")))
	#else
		#define CN_TEST_API
	#endif
	#define CN_GAME_API __attribute((visibility("default")))
#endif /* WIN32 */

/*
 * Assert and other debug functionality should be able to trigger a breakpoint
 * in the debugger.
 *
 * This behavior is exposed globally to allow breakpoints on non-fatal
 * conditions to be placed in source control for difficult bugs which may
 * require multiple check-ins to diagnose and fix.
 *
 * Use cases:
 * - assertion failure
 * - excessive frame time
*/
#if CN_DEBUG
	#if defined(_WIN32)
		#include <intrin.h>
		#define CN_DEBUG_BREAK() __debugbreak();
	#endif

	#if !defined(_WIN32)
		#include <signal.h>
		/**
		 * Use to force the debugger to stop at a specific line.
		 */
		#define CN_DEBUG_BREAK() raise(SIGTRAP)
	#endif
#endif

/*
 * CN_DEBUG_BREAK might be defined in release builds created with debug symbols
 * (e.g. RelWithDebugInfo).
 */
#ifndef CN_DEBUG_BREAK
	#define CN_DEBUG_BREAK() do {} while(0)
#endif

/*
 * Deprecation functionality.
 */

// From MSDN docs
// You can use the operators == (equality) and != (inequality) only to test for
// the bool values true or false.
#if CN_DEPRECATION_OMIT
	/*
	 * Don't declare deprecated APIs.
	 */
	#define CN_API_DEPRECATED(version, msg, decl)
#else
	/**
	 * Behavior might need to be removed.  Functionality to be removed should be
	 * marked, deprecated, and then removed in an understandable way.
	 *
	 * `CN_API_DEPRECATED` indicates an element is deprecated an will be removed
	 * after the given version.
	 */
	#define CN_API_DEPRECATED(version, msg, decl) decl
#endif

#if CN_DEPRECATION_BREAK
	/**
	 * Inline marker to appear within the source of a deprecated function to trigger
	 * a breakpoint to track down usage of deprecated functions.
	 */
	#define CN_WARN_DEPRECATED(msg) CN_DEBUG_BREAK()
#else
	#define CN_WARN_DEPRECATED(msg)
#endif

#if CN_TESTING
#include <calendon/test-asserts.h>
/**
 * Specialize the runtime assertion mechanism to perform a longjmp on assertion
 * failure, allowing assertions to be tested.
 */
#define CN_ASSERT(condition, message, ...) do { \
		if (!(condition)) { \
			if (cnTest_ExpectingAssert) { \
				longjmp(cnTest_AssertJumpBuffer, CN_TEST_ASSERTION_OCCURRED); \
			} \
			else { \
				printf("%s:%i Assertion failure: " message "\n", __FILE__, \
					__LINE__, ##__VA_ARGS__); \
				longjmp(cnTest_AssertUnexpectedJumpBuffer, CN_TEST_ASSERTION_UNEXPECTED); \
			} \
		} \
    } while (0)
#else
/**
 * Runtime assert mechanism.  `CN_ASSERT` is the preferred method of declaring
 * pre- and post-conditions within code, and also conditions which must be
 * true for correct code execution.
 *
 * `CN_ASSERT` is for declaring conditions which should only be violated as a
 * result of a bug, not an abnormal condition found during runtime (e.g. a
 * missing asset file).
*/
#define CN_ASSERT(condition, message, ...) do { \
		if (!(condition)) { \
			CN_FATAL_ERROR(message, ##__VA_ARGS__); \
		} \
    } while (0)
#endif

#define CN_ASSERT_NOT_NULL(value) CN_ASSERT(value, #value " is null.")
#define CN_ASSERT_FINITE_F32(value) CN_ASSERT(isfinite(value), #value " is not finite: %f", value)

/**
 * Implement a local version of static assert since `static_assert` is part of
 * C11.
 */
#define CN_STATIC_ASSERT(expr, message) \
	int static_assert_fn(int condition[!!(expr) ? 1 : -1])

/*
 * Reserve space statically to write a fatal error message when things go wrong.
 */
enum { fatalErrorBufferLength = 1024 };
extern CN_API char fatalErrorBuffer[fatalErrorBufferLength];

/*
 * An unrecoverable event happened at this point in the program.
 *
 * This causes a crash.  Use this when the program cannot recover from whatever
 * ill the program is in at this point.  Use `CN_FATAL_ERROR` to indicate
 * problems where the program was expected to succeed at an operation but
 * didn't, or an unrecoverable error occurred.
 */
#ifdef _WIN32
	#include <calendon/compat-windows.h>
	#include <debugapi.h>
	#define CN_FATAL_ERROR(error_message, ...) \
	    do { \
			if (IsDebuggerPresent()) { \
				printf(error_message, ##__VA_ARGS__); \
				fflush(stdout); \
				CN_DEBUG_BREAK(); \
				abort(); \
			} else { \
				snprintf(fatalErrorBuffer, fatalErrorBufferLength, "%s:%i\n" error_message, __FILE__, __LINE__, ##__VA_ARGS__); \
				MessageBox(NULL, fatalErrorBuffer, "Fatal Error", MB_OK); abort(); \
			} \
		} while (0)
#else
	#define CN_FATAL_ERROR(error_message, ...) \
		do { \
			printf(error_message, ##__VA_ARGS__); \
			fflush(stdout); \
			CN_DEBUG_BREAK(); \
			abort(); \
		} while (0)
#endif

/**
 * Used to suppress errors resulting from unused values.
 *
 * Typically this gets used during debugging to prevent from having to add and
 * remove function parameter names.  Long-term usage of `CN_UNUSED` is
 * discouraged and indicates that an API is likely to be deprecated or changed.
 */
#define CN_UNUSED(value) (void)(value)

/**
 * Recommends that a function be copied to the call site instead of called
 * (inlined).  Use sparingly and primarily for short functions.
 */
#define CN_INLINE inline

/*
 * Macro to be used while writing code to indicate that this code should never
 * be submitted for real.  Define to something meaningless in production to
 * trigger a compilation error.
 */
#if CN_DEBUG
	#define CN_DO_NOT_SUBMIT
#else
	#define CN_DO_NOT_SUBMIT production_code_has_do_not_submit
#endif

/**
 * Prevent mistakes of calculating array size by just making it a macro.
 */
#define CN_ARRAY_SIZE(arr) (sizeof(arr)  / sizeof(arr[0]))

#ifdef __cplusplus
}
#endif

#endif /* CN_H */
