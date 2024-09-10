# Minimal makefile for Sphinx documentation
#

# -- 需要手动添加的项目名称module_name和输出前缀output_file_prefix和文档语言LANG-------
# 【参考格式】module_name  := BitRateControlApplicationNotes
# 【参考格式】output_file_prefix := BitRateControlApplicationNotes
# 【注意事项】module_name === project in conf.py, use \_ instead of _ if module name contains _
module_name  := TDLSDKSoftwareDevelopmentGuide
output_file_prefix := TDLSDKSoftwareDevelopmentGuide
LANG := zh
pdf_name  = $(output_file_prefix)_$(LANG).pdf

# You can set these variables from the command line, and also
# from the environment for the first two.
SPHINXOPTS    ?= -W -n
SPHINXBUILD   ?= sphinx-build
SOURCEDIR     = source
BUILDDIR      = build

# Put it first so that "make" without argument is like "make help".
help:
	@$(SPHINXBUILD) -M help "$(SOURCEDIR)" "$(BUILDDIR)" $(SPHINXOPTS) $(O)

.PHONY: help Makefile

# Catch-all target: route all unknown targets to Sphinx using the new
# "make mode" option.  $(O) is meant as a shortcut for $(SPHINXOPTS).
%: Makefile
	@$(SPHINXBUILD) -M $@ "$(SOURCEDIR)" "$(BUILDDIR)" $(SPHINXOPTS) $(O)

# -- 需要手动添加的PHONY Target-----------------------------------------
## make html：渲染成html格式
## make pdf：渲染成pdf格式
## make clean：清楚 /build 所有生成文件
pdf: latex
	@cd $(BUILDDIR)/latex && xelatex $(module_name).tex
	@cd $(BUILDDIR)/latex && xelatex $(module_name).tex
	@mv $(BUILDDIR)/latex/$(module_name).pdf $(BUILDDIR)/"${pdf_name}" && rm -rf $(BUILDDIR)/latex

web: html
	#@python3 -m http.server --directory build/html

clean:
	@rm -rf $(BUILDDIR)
