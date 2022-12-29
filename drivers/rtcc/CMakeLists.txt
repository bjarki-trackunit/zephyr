# Copyright 2022 Bjarki Arge Andreasen
# SPDX-License-Identifier: Apache-2.0

zephyr_library()
zephyr_library_sources_ifdef(CONFIG_RTCC_EMUL rtcc_emul.c)
