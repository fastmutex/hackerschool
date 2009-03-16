// -*- mode: C++; tab-width: 4; indent-tabs-mode: nil -*- (for GNU Emacs)
//
// $Id: config.h,v 1.1.1.1 2002/08/08 12:49:37 dev Exp $

#ifndef _config_h_
#define _config_h_

/*
 * name of rules file
 */
#define RULES_NAME			L"\\SystemRoot\\system32\\drivers\\etc\\ndis_fw.conf"

/*
 * maximum size of rules file
 */
#define MAX_RULES_SIZE		(128 * 1024)

/*
 * name prefix of log file
 */
#define LOG_NAME			L"\\SystemRoot\\system32\\LogFiles\\ndis_fw\\"

/*
 * internal logging queue elements count
 */
#define LOG_QUEUE_SIZE	100

#endif
