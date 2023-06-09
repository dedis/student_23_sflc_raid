// MinUnit -- a minimal unit testing framework for C. Slightly modified.
// https://jera.com/techinfo/jtns/jtn002

#ifndef _MINUNIT_H_
#define _MINUNIT_H_

#define mu_assert(message, test) do { if (!(test)) return message; } while (0)
#define mu_run_test(test) do { char *message = test(); if (message) return message; } while (0)

#endif	// _MINUNIT_H_
