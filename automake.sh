#/usr/bin/env bash
#
#https://blog.csdn.net/helloxiaozhe/article/details/80940066
#automake.sh 自动化编译脚本
#
main() {
	rootpath=/home/Landy/TianXia/Program
	echo '编译根路径为 '${rootpath}
	echo '---------------------------------------------------------------------------'
	i=0
	while IFS=$'\n' read -r projpath; do
		if [[ -d "${rootpath}/$projpath" ]]; then
			proj[$i]=${rootpath}/$projpath
			i=`expr $i + 1`
			echo ${rootpath}/$projpath' [OK]'
		else
			echo ${rootpath}/$projpath' [FAILED]'
		fi
	done < projlist.md
	if [[ ! -x "${rootpath}/make.log" ]]; then
		mkdir -p '${rootpath}/make.log'
	fi
	rm -rf ${rootpath}/make.log/*
	c=0
	for i in "${!proj[@]}"; do
		if [[ -f "${proj[$i]}/Makefile" ]]; then
			echo '---------------------------------------------------------------------------'
			printf '当前编译项目路径为 proj[%d] %s !!!\n' ${i} ${proj[$i]}
			#echo '---------------------------------------------------------------------------' >> ${rootpath}/make.log/list.txt
			#printf '当前编译项目路径为 proj[%d] %s !!!\n' ${i} ${proj[$i]} >> ${rootpath}/make.log/list.txt
			c=`expr $c + 1`
			cd ${proj[$i]}
			make clean
			#make > ${rootpath}/make.log/${i}'.log'
			make
		else
			echo '找不到 '${proj[$i]}'/Makefile'
		fi
	done
	#echo '总共编译了 '${c}' 个项目!!!'  >> ${rootpath}/make.log/list.txt
	echo '总共编译了 '${c}' 个项目!!!'
}
main
