#
# Copyright (C) 2011 OpenWrt.org
#

. /lib/functions/system.sh

PART_NAME=firmware
RAMFS_COPY_DATA='/etc/fw_env.config /var/lock/fw_printenv.lock'
RAMFS_COPY_BIN='nandwrite fw_printenv fw_setenv'
REQUIRE_IMAGE_METADATA=1

redboot_fis_do_upgrade() {
	local append
	local sysup_file="$1"
	local kern_part="$2"
	local magic=$(get_magic_word "$sysup_file")

	if [ "$magic" = "4349" ]; then
		local kern_length=0x$(dd if="$sysup_file" bs=2 skip=1 count=4 2>/dev/null)

		[ -f "$UPGRADE_BACKUP" ] && append="-j $UPGRADE_BACKUP"
		dd if="$sysup_file" bs=64k skip=1 2>/dev/null | \
			mtd -r $append -F$kern_part:$kern_length:0x80060000,rootfs write - $kern_part:rootfs

	elif [ "$magic" = "7379" ]; then
		local board_dir=$(tar tf $sysup_file | grep -m 1 '^sysupgrade-.*/$')
		local kern_length=$(tar xf $sysup_file ${board_dir}kernel -O | wc -c)

		[ -f "$UPGRADE_BACKUP" ] && append="-j $UPGRADE_BACKUP"
		tar xf $sysup_file ${board_dir}kernel ${board_dir}root -O | \
			mtd -r $append -F$kern_part:$kern_length:0x80060000,rootfs write - $kern_part:rootfs

	else
		echo "Unknown image, aborting!"
		return 1
	fi
}

platform_check_image() {
	return 0
}

platform_do_upgrade() {
	local board=$(board_name)

	case "$board" in
	adtran,bsap1800-v2|\
	adtran,bsap1840)
		redboot_fis_do_upgrade "$1" vmlinux_2
		;;
	adtran,bsap1920|\
	adtran,bsap1925)
		if [ $(grep -ow 'root=31:0[68]' /proc/cmdline) = "root=31:08" ]
		then
			PART_NAME=firmware1
			fw_setenv boot_bank A
		else
			PART_NAME=firmware2
			fw_setenv boot_bank B
		fi
		default_do_upgrade "$1"
		;;
	jjplus,ja76pf2)
		redboot_fis_do_upgrade "$1" linux
		;;
	ubnt,routerstation|\
	ubnt,routerstation-pro)
		redboot_fis_do_upgrade "$1" kernel
		;;
	*)
		default_do_upgrade "$1"
		;;
	esac
}
