// -*- mode: C++; tab-width: 4; indent-tabs-mode: nil -*- (for GNU Emacs)
//
// $Id: log.h,v 1.1.1.1 2002/08/08 12:49:37 dev Exp $

#ifndef _log_h_
#define _log_h_

NTSTATUS	open_log(void);
void		close_log(void);

void		log_str(const char *fmt, ...);
void		log_str_dispatch(const char *fmt, ...);

#endif
