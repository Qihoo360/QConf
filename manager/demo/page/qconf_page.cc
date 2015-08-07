#include "stdlib.h"
#include "cgic.h"
#include <string>
#include <vector>
using namespace std;


void generate(std::string path, std::string value, std::string idc, std::vector<std::string> idcs, std::vector<std::string> children, std::string target)
{
fprintf(cgiOut, "<!DOCTYPE html>");
fprintf(cgiOut, "<!--[if lt IE 7 ]><html lang=\"en\" class=\"ie6 ielt7 ielt8 ielt9\"><![endif]--><!--[if IE 7 ]><html lang=\"en\" class=\"ie7 ielt8 ielt9\"><![endif]--><!--[if IE 8 ]><html lang=\"en\" class=\"ie8 ielt9\"><![endif]--><!--[if IE 9 ]><html lang=\"en\" class=\"ie9\"> <![endif]--><!--[if (gt IE 9)|!(IE)]><!-->");
fprintf(cgiOut, "<html lang=\"en\"><!--<![endif]-->");
fprintf(cgiOut, "<head>");
fprintf(cgiOut, "<meta charset=\"utf-8\">");
fprintf(cgiOut, "<title>QConf - Master</title>");
fprintf(cgiOut, "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">");
fprintf(cgiOut, "<link href=\"css/bootstrap.min.css\" rel=\"stylesheet\">");
fprintf(cgiOut, "<link href=\"css/bootstrap-responsive.min.css\" rel=\"stylesheet\">");
fprintf(cgiOut, "<link href=\"css/site.css\" rel=\"stylesheet\">");
fprintf(cgiOut, "<!--[if lt IE 9]><script src=\"http://html5shim.googlecode.com/svn/trunk/html5.js\"></script><![endif]-->");
fprintf(cgiOut, "</head>");
fprintf(cgiOut, "<body>");
fprintf(cgiOut, "<div class=\"container\">");
fprintf(cgiOut, "<div class=\"row\">");
fprintf(cgiOut, "<div class=\"span9\">");
fprintf(cgiOut, "<div class=\"hero-unit\">");
fprintf(cgiOut, "<h1>");
fprintf(cgiOut, "QConf Manager");
fprintf(cgiOut, "</h1>");
fprintf(cgiOut, "<p>");
fprintf(cgiOut, "QConf Management Interface.");
fprintf(cgiOut, "</p>");
fprintf(cgiOut, "</div>");
fprintf(cgiOut, "<h2>");
fprintf(cgiOut, "QConf Node Infomation");
fprintf(cgiOut, "</h2>");
fprintf(cgiOut, "<hr/>");
fprintf(cgiOut, "<form id=\"edit-profile\" class=\"form-horizontal\" method=\"POST\">");
fprintf(cgiOut, "<fieldset>");
fprintf(cgiOut, "<div class=\"control-group\">");
fprintf(cgiOut, "<label class=\"control-label\" for=\"input01\">Path</label>");
fprintf(cgiOut, "<div class=\"controls\">");
fprintf(cgiOut, "<input type=\"text\" class=\"input-xlarge\" name=\"path\" value=\"");
cgiHtmlEscape(path.c_str());
fprintf(cgiOut, "\" />&nbsp;&nbsp;&nbsp;&nbsp;");
fprintf(cgiOut, "<button type=\"submit\" name=\"check_node\" class=\"btn btn-primary\"  >Check</button>");
fprintf(cgiOut, "</div>");
fprintf(cgiOut, "</div>");
fprintf(cgiOut, "<div class=\"control-group\">");
fprintf(cgiOut, "<label class=\"control-label\" for=\"\">Idc</label>");
fprintf(cgiOut, "<div class=\"controls\">");
fprintf(cgiOut, "<select name=\"idc\" class=\"input-xlarge\" >");
vector<string>::iterator it = idcs.begin();
for(; it != idcs.end(); ++it)
{
fprintf(cgiOut, "<option value=\"");
cgiHtmlEscape((*it).c_str());
fprintf(cgiOut, "\"");
if ((*it) == idc)
{
fprintf(cgiOut, "selected");
}
fprintf(cgiOut, ">");
cgiHtmlEscape((*it).c_str());
fprintf(cgiOut, "</option>");
}
fprintf(cgiOut, "</select>");
fprintf(cgiOut, "</div>");
fprintf(cgiOut, "</div>");
fprintf(cgiOut, "<div class=\"control-group\">");
fprintf(cgiOut, "<label class=\"control-label\" for=\"textarea\">Value</label>");
fprintf(cgiOut, "<div class=\"controls\">");
fprintf(cgiOut, "<textarea name=\"value\" class=\"input-xlarge\" id=\"textarea\" rows=\"15\" style=\"width:80%\" >");
cgiHtmlEscape(value.c_str());
fprintf(cgiOut, "</textarea>");
fprintf(cgiOut, "</div>");
fprintf(cgiOut, "</div>");
fprintf(cgiOut, "<div class=\"form-actions\">");
fprintf(cgiOut, "<button type=\"submit\" name=\"modify_node\" class=\"btn btn-primary\" onclick='var r=confirm(\"Are you sure you want to modify or add this node?\"); return r;' >Add Or Modify</button>&nbsp;&nbsp;&nbsp;");
fprintf(cgiOut, "<button type=\"submit\" name=\"delete_node\" class=\"btn\" onclick='var r=confirm(\"Are you sure you want to delete this node?\"); return r;'>Delete</button>");
fprintf(cgiOut, "</div>");
fprintf(cgiOut, "</fieldset>");
fprintf(cgiOut, "</form>");
fprintf(cgiOut, "<h2>");
fprintf(cgiOut, "Children Nodes");
fprintf(cgiOut, "</h2>");
fprintf(cgiOut, "<table class=\"table table-bordered table-striped\">");
fprintf(cgiOut, "<thead>");
fprintf(cgiOut, "<tr>");
fprintf(cgiOut, "<th>");
fprintf(cgiOut, "Path");
fprintf(cgiOut, "</th>");
fprintf(cgiOut, "<th>");
fprintf(cgiOut, "Idc");
fprintf(cgiOut, "</th>");
fprintf(cgiOut, "<th>");
fprintf(cgiOut, "View");
fprintf(cgiOut, "</th>");
fprintf(cgiOut, "</tr>");
fprintf(cgiOut, "</thead>");
fprintf(cgiOut, "<tbody>");
vector<string>::iterator cit = children.begin();
for(; cit != children.end(); ++cit)
{
fprintf(cgiOut, "<tr>");
fprintf(cgiOut, "<td>");
fprintf(cgiOut, "");
cgiHtmlEscape((*cit).c_str());
fprintf(cgiOut, "");
fprintf(cgiOut, "</td>");
fprintf(cgiOut, "<td>");
fprintf(cgiOut, "");
cgiHtmlEscape(idc.c_str());
fprintf(cgiOut, "");
fprintf(cgiOut, "</td>");
fprintf(cgiOut, "<td>");
fprintf(cgiOut, "<a href=\"");
cgiHtmlEscape(target.c_str());
fprintf(cgiOut, "");
fprintf(cgiOut, "?path=");
cgiHtmlEscape((*cit).c_str());
fprintf(cgiOut, "");
fprintf(cgiOut, "&idc=");
cgiHtmlEscape(idc.c_str());
fprintf(cgiOut, "\" class=\"view-link\">View</a>");
fprintf(cgiOut, "</td>");
fprintf(cgiOut, "</tr>");
}
fprintf(cgiOut, "</tbody>");
fprintf(cgiOut, "</table>");
fprintf(cgiOut, "<h2>");
fprintf(cgiOut, "Parent Node");
fprintf(cgiOut, "</h2>");
fprintf(cgiOut, "<table class=\"table table-bordered table-striped\">");
fprintf(cgiOut, "<thead>");
fprintf(cgiOut, "<tr>");
fprintf(cgiOut, "<th>");
fprintf(cgiOut, "Path");
fprintf(cgiOut, "</th>");
fprintf(cgiOut, "<th>");
fprintf(cgiOut, "Idc");
fprintf(cgiOut, "</th>");
fprintf(cgiOut, "<th>");
fprintf(cgiOut, "View");
fprintf(cgiOut, "</th>");
fprintf(cgiOut, "</tr>");
fprintf(cgiOut, "</thead>");
fprintf(cgiOut, "<tbody>");
string parent = (path.substr(0, path.find_last_of("/")));
if (!parent.empty())
{
fprintf(cgiOut, "<tr>");
fprintf(cgiOut, "<td>");
fprintf(cgiOut, "");
cgiHtmlEscape(parent.c_str());
fprintf(cgiOut, "");
fprintf(cgiOut, "</td>");
fprintf(cgiOut, "<td>");
fprintf(cgiOut, "");
cgiHtmlEscape(idc.c_str());
fprintf(cgiOut, "");
fprintf(cgiOut, "</td>");
fprintf(cgiOut, "<td>");
fprintf(cgiOut, "<a href=\"");
cgiHtmlEscape(target.c_str());
fprintf(cgiOut, "");
fprintf(cgiOut, "?path=");
cgiHtmlEscape(parent.c_str());
fprintf(cgiOut, "");
fprintf(cgiOut, "&idc=");
cgiHtmlEscape(idc.c_str());
fprintf(cgiOut, "\" class=\"view-link\">View</a>");
fprintf(cgiOut, "</td>");
fprintf(cgiOut, "</tr>");
}
fprintf(cgiOut, "</tbody>");
fprintf(cgiOut, "</table>");
fprintf(cgiOut, "<ul class=\"pager\">");
fprintf(cgiOut, "<li class=\"next\">");
fprintf(cgiOut, "More Infomation <a href=\"https://github.com/Qihoo360/QConf\" target=\"_blank\" title=\"github\">Github</a>  <a href=\"https://github.com/Qihoo360/QConf/wiki\" title=\"wiki\" target=\"_blank\">wiki</a>");
fprintf(cgiOut, "</li>");
fprintf(cgiOut, "</ul>");
fprintf(cgiOut, "</div>");
fprintf(cgiOut, "</div>");
fprintf(cgiOut, "</div>");
fprintf(cgiOut, "<script src=\"js/jquery.min.js\"></script>");
fprintf(cgiOut, "<script src=\"js/bootstrap.min.js\"></script>");
fprintf(cgiOut, "<script src=\"js/site.js\"></script>");
fprintf(cgiOut, "</body>");
fprintf(cgiOut, "</html>");
}
