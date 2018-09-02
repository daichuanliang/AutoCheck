#!/bin/bash

export BUILD="false"

if [ "${BUILD}" == "true" ]; then
    #repo init -u ssh://android.intel.com/manifest -b platform/android/main -m android-main
    #repo init -m snapshot_mainline_1223.xml
    #repo sync -j4

    source build/envsetup.sh
    lunch redhookbay-eng
    #lunch byt_t_ffrd8-eng
    make flashfiles -j4 2>&1 | tee build.out
fi

export DATABASE="true" #"true"
export readonly database_no_ctags="false"
export readonly database_no_cscope="false"
export readonly database_no_bob="true"

export readonly projlist_orig=( \
"." \
)

#"external" \
#"linux-3.10/drivers/media/i2c/soc_camera" \
#"linux-3.10/drivers/media/platform/xgold" \
#"linux-3.10/drivers/staging" \
#"development" \
#"external" \
#"packages/apps/Camera" \
#"linux-3.10/include" \
#"linux-3.10/drivers/staging" \
#"linux-3.10/arch/x86" \

#"cts" \


export readonly projlist_bob=( \
"device" \
"frameworks/av" \
"hardware" \
"system" \
"vendor/intel/hardware/libcamera2" \
)

if [ "${DATABASE}" == "true" ]; then
    #------------filter out invalid proj-------------
    proj_len_orig=${#projlist_orig[*]}
    let idx=0;
    for ((i=0; i<${proj_len_orig}; i++)); do
        folder=${projlist_orig[${i}]}
        if [ -e ${folder} ]; then
            projlist[$idx]=$folder
            let idx=$idx+1;
        else
            echo "Invlid proj path: $folder"
        fi
    done

    echo "${projlist[*]} "
    proj_len=${#projlist[*]}
    echo "Length of projlist: ${proj_len} "

    #------------generate profile-------------
    :>projfile
    :>projfile-more
    for ((i=0; i<${proj_len}; i++)); do
        folder=${projlist[${i}]}
        find "${folder}" -type f -a \( -name "*.c" -o -name "*.h" -o -name "*.cpp" -o -name "*.java" -o -name "*.py" -o -name "*.mk" -o -name "Makefile" -o -name "*.rc"  -o -name "*.conf" -o -name "*.dtsi" -o -name "*.dts" \) |grep -iv "toolchain" |grep -iv "labtool" >> projfile
        #find "${folder}" -type f -regex '.*\.\(cpp\|h\|c\|java\|xml\|mk\|rc\|conf\|dtsi\|dts\)' |grep -iv "toolchain" |grep -iv "labtool" >> projfile
    done

    #------------generate cscope-------------
    cat projfile|egrep '(.*\.cpp|.*\.c|.*\.h|.*\.java|.*\.cxx|.*\.py)$'>projfile_tags

    if [ ! "${database_no_cscope}" == "true" ]; then
        #-----cscope----------
        rm -f {./cscope.out,./cscope.*.out}
        cscope -Rbkq -i projfile_tags
        echo "cscope done"
    fi

    #------------generate ctags-------------
    if [ ! "${database_no_ctags}" == "true" ]; then
        #-----ctags----------
        rm -rf ./tags
        ctags -R --c++-kinds=+p --fields=+iaS --extra=+q -L projfile_tags
        echo "ctags done"
    fi

    #------------generate lookupfile-------------
    :>lookupfile
    echo -e "!_TAG_FILE_SORTED\t2\t/2=foldcase/" > lookupfile;
    :>lookupfile_unsort
    for ((i=0; i<${proj_len}; i++)); do
        folder=${projlist[${i}]}
        #find "${folder}" -type f -printf "%f\t%p\t1\n" >> lookupfile_unsort
        find "${folder}" -type f -regex '.*\.\(cpp\|h\|c\|java\)'  -printf "%f\t%p\t1\n" >> lookupfile_unsort
    done
    sort -f lookupfile_unsort >> lookupfile

    #rm projfile_tags

    #********************************************************
    if [ ! "${database_no_bob}" == "true" ]; then
        :>projfile_bob
        proj_len=${#projlist_bob[*]}
        for ((i=0; i<${proj_len}; i++)); do
            folder=${projlist_bob[${i}]}
            find "${folder}" -type f -a \( -name "*.c" -o -name "*.h" -o -name "*.cpp" -o -name "*.java" -o -name "*.cxx" \) |grep -iv "toolchain" |grep -iv "labtool" >> projfile_bob
        done
        bob -L projfile_bob --call-tags
        echo "bob done"
    fi
fi


