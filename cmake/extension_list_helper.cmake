# SPDX-FileCopyrightText: 2021 Arthur Brainville (Ybalrid) <ybalrid@ybalrid.info>
#
# SPDX-License-Identifier: MIT

message(STATUS "loaded extension list helper")
function (build_instance_extension_list ext_names ext_versions output_list)
	list(LENGTH ext_names nb_extensions)
	list(LENGTH ext_versions nb_extensions)

	if(nb_extensions GREATER 0)
		set(instance_extension_list_local ",\n    \"instance_extensions\" : [\n")
		set(i 0)
		while(i LESS nb_extensions)
			list(GET ext_names ${i} current_ext_name)
			list(GET ext_versions ${i} current_ext_version)

			string(CONCAT instance_extension_list_local ${instance_extension_list_local} "      {\n")
			string(CONCAT instance_extension_list_local ${instance_extension_list_local} "        \"name\" : \"${current_ext_name}\",\n")
			string(CONCAT instance_extension_list_local ${instance_extension_list_local} "        \"extension_version\" : ${current_ext_version}\n")
			string(CONCAT instance_extension_list_local ${instance_extension_list_local} "      }")

			math(EXPR last_one ${nb_extensions}-1)
			if(i EQUAL last_one)
				string(CONCAT instance_extension_list_local ${instance_extension_list_local} "\n")
			else()
				string(CONCAT instance_extension_list_local ${instance_extension_list_local} ",\n")
			endif()

			math(EXPR i ${i}+1)
		endwhile()
		string(CONCAT instance_extension_list_local "${instance_extension_list_local}" "    ]")
	endif()
	#message(STATUS ${instance_extension_list_local})
	set("${output_list}" "${instance_extension_list_local}" PARENT_SCOPE)
endfunction()

function(build_instance_extension_header ext_names ext_versions output_list)
	list(LENGTH ext_names nb_extensions)
	list(LENGTH ext_versions nb_extensions)

	if(nb_extensions GREATER 0)
		set(instance_extension_list_local "#define XR_THISLAYER_HAS_EXTENSIONS true\nstatic const char * layer_extension_names[]  = {\n")
		set(i 0)
		while(i LESS nb_extensions)
			list(GET ext_names ${i} current_ext_name)
			string(CONCAT instance_extension_list_local "${instance_extension_list_local}" "\"${current_ext_name}\",\n")
			math(EXPR i ${i}+1)
		endwhile()
		string(CONCAT instance_extension_list_local "${instance_extension_list_local}" "};//")

	else()
		set(instance_extension_list_local "#define XR_THISLAYER_HAS_EXTENSIONS false\n")
	endif()
	set("${output_list}" ${instance_extension_list_local} PARENT_SCOPE)
endfunction()
