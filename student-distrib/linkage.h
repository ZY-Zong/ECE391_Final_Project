//
// Created by liuzikai on 10/26/19.
//

#ifndef _LINKAGE_H
#define _LINKAGE_H

#define asmlinkage  __attribute__((regparm(0)))
#define fastcall	__attribute__((regparm(3)))

#define __user

#endif // _LINKAGE_H
