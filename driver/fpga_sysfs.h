/**
  * Copyright (c) 2021, Systems Group, ETH Zurich
  * All rights reserved.
  *
  * Redistribution and use in source and binary forms, with or without modification,
  * are permitted provided that the following conditions are met:
  *
  * 1. Redistributions of source code must retain the above copyright notice,
  * this list of conditions and the following disclaimer.
  * 2. Redistributions in binary form must reproduce the above copyright notice,
  * this list of conditions and the following disclaimer in the documentation
  * and/or other materials provided with the distribution.
  * 3. Neither the name of the copyright holder nor the names of its contributors
  * may be used to endorse or promote products derived from this software
  * without specific prior written permission.
  *
  * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
  * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
  * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
  * IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
  * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
  * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
  * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
  * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
  * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
  */

#ifndef __FPGA_SYSFS_H__
#define __FPGA_SYSFS_H__

#include "coyote_dev.h"

/* Sysfs */

// IP
ssize_t cyt_attr_ip_q0_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf);
ssize_t cyt_attr_ip_q0_store(struct kobject *kobj, struct kobj_attribute *attr, const char *buf, size_t count);

ssize_t cyt_attr_ip_q1_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf);
ssize_t cyt_attr_ip_q1_store(struct kobject *kobj, struct kobj_attribute *attr, const char *buf, size_t count);

// MAC
ssize_t cyt_attr_mac_q0_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf);
ssize_t cyt_attr_mac_q0_store(struct kobject *kobj, struct kobj_attribute *attr, const char *buf, size_t count);

ssize_t cyt_attr_mac_q1_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf);
ssize_t cyt_attr_mac_q1_store(struct kobject *kobj, struct kobj_attribute *attr, const char *buf, size_t count);

// Stats
ssize_t cyt_attr_nstats_q0_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf);
ssize_t cyt_attr_nstats_q1_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf);
ssize_t cyt_attr_xstats_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf);

// Config
ssize_t cyt_attr_cnfg_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf);

#endif // FPGA FOPS