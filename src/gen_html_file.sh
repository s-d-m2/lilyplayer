#!/usr/bin/env bash


set -eux
set -o pipefail

function usage() {
    local readonly msg
    msg="Usage: $0 <params>

params:

--html-template <html template file>
		This parameter is required

--zstd-js-file  <zstd.js file>
		This parameter is required

--output-file   <output html file>
		This parameter is required

--compressed-wasm-file <path to zstd compressed wasm file>
		This parameter is required

--uncompressed-wasm-file <path to wasm file>
		This parameter is required


--gawk-command  <gawk command>
		This parameter is optional
		Default value is \"awk\"
"
    echo "${msg}"
}

function get_file_size() {
    local readonly filename="$1"

    stat -c '%s' "${filename}"
}

function create_output_file() {
    local readonly html_template_file="$1"
    local readonly zstd_js_file="$2"
    local readonly output_file="$3"
    local readonly gawk_cmd="$4"
    local readonly compressed_wasm_file="$5"
    local readonly uncompressed_wasm_file="$6"

    local readonly uncompressed_size
    uncompressed_size="$(get_file_size "${uncompressed_wasm_file}")"

    local readonly compressed_size
    compressed_size="$(get_file_size "${compressed_wasm_file}")"


    "$gawk_cmd" \
	-v uncompressed_size="${uncompressed_size}" \
	-v compressed_size="${compressed_size}" \
	'
BEGIN {
  while (getline <ARGV[2]) {
    zstd_js = zstd_js $0 "\n"
  }
  delete ARGV[2];
}

(NF == 1) && ($1 == "<!--Insert-ZSTD-JS-HERE-->") {

  print "    <script type='"'"'text/javascript'"'"'>"
  print "      " zstd_js
  print "    </script>"
  next
}

(NF == 5) && ($(NF - 1) == "/*INSERT-uncompressed-here*/") {
 $(NF - 1) = uncompressed_size
  print
  next
}

(NF == 5) && ($(NF - 1) == "/*INSERT-compressed-here*/") {
 $(NF - 1) = compressed_size
  print
  next
}

{
  print
}
' "${html_template_file}" "${zstd_js_file}" > "${output_file}"

}


function main() {
    local html_template_file=''
    local zstd_js_file=''
    local output_file=''
    local compressed_wasm_file=''
    local uncompressed_wasm_file=''
    local print_help='false'
    local gawk_cmd='awk'
    local has_error='false'
    local err_msg=''

    while [ $# -ge 1 ] ; do
	case "$1" in
	    '--html-template')
		html_template_file="$2"
		shift 2
		;;
	    '--zstd-js-file')
		zstd_js_file="$2"
		shift 2
		;;
	    '--output-file')
		output_file="$2"
		shift 2
		;;
	    '--gawk-command')
		gawk_cmd="$2"
		shift 2
		;;
	    '--help')
		print_help='true'
		shift 1
		;;

	    '--compressed-wasm-file')
		compressed_wasm_file="$2"
		shift 2
		;;

	    '--uncompressed-wasm-file')
		uncompressed_wasm_file="$2"
		shift 2
		;;

	    *)
		has_error='true'
		err_msg="${err_msg}Unknown parameter $1\n"
		shift
		;;
	esac
    done

    if [ -z "${html_template_file}" ] ; then
	err_msg="${err_msg}Error: --html-template <input_template file> is required\n"
	has_error='true'
    fi

    if [ -z "${zstd_js_file}" ] ; then
	err_msg="${err_msg}Error: --zstd-js-file <input zstd js file> is required\n"
	has_error='true'
    fi

    if [ -z "${output_file}" ] ; then
	err_msg="${err_msg}Error: --output-file <output file> is required\n"
	has_error='true'
    fi

    if [ -z "${compressed_wasm_file}" ] ; then
	err_msg="${err_msg}Error: --compressed-wasm-file <zstd compressed wasm file> is required\n"
	has_error='true'
    fi

    if [ -z "${uncompressed_wasm_file}" ] ; then
	err_msg="${err_msg}Error: --uncompressed-wasm-file <wasm file> is required\n"
	has_error='true'
    fi

    if [ "${has_error}" = 'true' ] ; then
	usage >&2
	echo '' >&2
	echo '' >&2
	echo "${err_msg}" >&2
	return 2
    fi

    if [ "${print_help}" = 'true' ] ; then
	usage
	return
    fi

    create_output_file \
	"${html_template_file}" \
	"${zstd_js_file}" \
	"${output_file}" \
	"${gawk_cmd}" \
	"${compressed_wasm_file}" \
	"${uncompressed_wasm_file}"
}

main "$@"
