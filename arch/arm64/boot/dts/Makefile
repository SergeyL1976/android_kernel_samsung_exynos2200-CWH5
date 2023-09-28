# SPDX-License-Identifier: GPL-2.0
dtbo-y += samsung/b0s/b0s_eur_openx_w01_r19.dtbo
dtbo-y += samsung/b0s/b0s_eur_openx_w01_r26.dtbo
dtbo-y += samsung/b0s/b0s_eur_openx_w01_r28.dtbo
dtbo-y += samsung/b0s/b0s_eur_openx_w01_r21.dtbo
dtbo-y += samsung/b0s/b0s_eur_openx_w01_r24.dtbo
dtbo-y += samsung/b0s/b0s_eur_openx_w01_r27.dtbo
dtbo-y += samsung/g0s/g0s_eur_openx_w01_r25.dtbo
dtbo-y += samsung/g0s/g0s_eur_openx_w01_r23.dtbo
dtbo-y += samsung/g0s/g0s_eur_openx_w01_r19.dtbo
dtbo-y += samsung/g0s/g0s_eur_openx_w01_r20.dtbo
dtbo-y += samsung/g0s/g0s_eur_openx_w01_r21.dtbo
dtbo-y += samsung/g0s/g0s_eur_openx_w01_r27.dtbo
dtbo-y += samsung/g0s/g0s_eur_openx_w01_r26.dtbo
dtbo-y += samsung/g0s/g0s_eur_openx_w01_r24.dtbo
dtbo-y += samsung/r0s/r0s_eur_openx_w01_r24.dtbo
dtbo-y += samsung/r0s/r0s_eur_openx_w01_r19.dtbo
dtbo-y += samsung/r0s/r0s_eur_openx_w01_r26.dtbo
dtbo-y += samsung/r0s/r0s_eur_openx_w01_r23.dtbo
dtbo-y += samsung/r0s/r0s_eur_openx_w01_r20.dtbo
dtbo-y += samsung/r0s/r0s_eur_openx_w01_r21.dtbo
dtbo-y += samsung/r0s/r0s_eur_openx_w01_r25.dtbo
dtbo-y += samsung/r0s/r0s_eur_openx_w01_r27.dtbo
dtbo-y += exynos/board/univ/s5e9925_evt0-universal9925_rev00.dtbo
dtbo-y += exynos/board/univ/s5e9925-universal9925_rev00.dtbo
dtbo-y += exynos/board/univ/s5e9925-universal9925_rev1b.dtbo
dtbo-y += exynos/board/univ/s5e9925-universal9925_rev17.dtbo
dtbo-y += exynos/board/univ/s5e8825-universal8825_r00.dtbo
dtbo-y += exynos/board/erd/s5e9925-erd9925.dtbo
dtbo-y += exynos/board/erd/s5e8825-erd8825_r10.dtbo
dtbo-y += exynos/board/erd/s5e9925-erd9925_r18.dtbo
dtbo-y += exynos/board/erd/s5e9925_evt0-erd9925.dtbo
dtbo-y += exynos/board/erd/s5e9925_s6165-erd9925.dtbo
dtbo-y += exynos/board/erd/s5e9925_evt0_s5300-erd9925.dtbo
dtbo-y += exynos/board/erd/s5e9925-erd9925_r13.dtbo
dtbo-y += exynos/board/erd/s5e8825-erd8825.dtbo
dtbo-y += exynos/board/erd/s5e8825-erd8825_r11.dtbo
dtbo-y += exynos/board/erd/s5e9925_s6165-erd9925_r17.dtbo
dtbo-y += exynos/board/erd/s5e9925_evt0-erd9925_r11.dtbo
dtbo-y += exynos/board/erd/s5e9925_evt0_s5300-erd9925_r11.dtbo
dtb-y += exynos/exynos2100.dtb
dtb-y += exynos/exynos5433-tm2.dtb
dtb-y += exynos/exynos5433-tm2e.dtb
dtb-y += exynos/exynos7-espresso.dtb
dtb-y += exynos/s5e8825.dtb
dtb-y += exynos/s5e9925_evt0.dtb
dtb-y += exynos/s5e9925.dtb

targets += dtbs
DTB_LIST  := $(dtb-y) $(dtbo-y)
always    := $(DTB_LIST)

dtbs: $(addprefix $(obj)/, $(DTB_LIST))

clean-files := *.dtb*
