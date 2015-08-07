#!/usr/bin/python
# This script is used to generate qconf_page.cc file from qconf_page.html
f = open('qconf_page.html')
f_out = open('qconf_page.cc', 'wa')

f_out.write("#include \"stdlib.h\"\n")
f_out.write("#include \"cgic.h\"\n")
f_out.write("#include <string>\n")
f_out.write("#include <vector>\n")
f_out.write("using namespace std;\n")
f_out.write("\n\n")

f_out.write("void generate(std::string path, std::string value, std::string idc, std::vector<std::string> idcs, std::vector<std::string> children, std::string target)\n")
f_out.write("{\n")
in_code = False
for line in f.readlines():
    line = line.strip('\r\n')
    line = line.strip()
    start = line.find('<#')
    end = line.find('#>')
    if start != -1 and end != -1:
        variable = line[start + 2:end]
        before = line[:start];
        end = line[end + 2:]
        before_c = before.replace("\"", "\\\"");
        end_c = end.replace("\"", "\\\"");
        f_out.write("fprintf(cgiOut, \"" + before_c + "\");\n")
        f_out.write("cgiHtmlEscape(" + variable + ");\n")
        f_out.write("fprintf(cgiOut, \"" + end_c + "\");\n")
        continue
    
    if line.find('<%') != -1:
        in_code = True
        continue
    if line.find('%>') != -1:
        in_code = False
        continue
    if in_code is True:
        f_out.write(line + "\n")
    else:
        content = line.replace("\"", "\\\"");
        f_out.write("fprintf(cgiOut, \"" + content + "\");\n")
f_out.write("}\n")
f.close()
f_out.close()
