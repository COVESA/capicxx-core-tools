# Creating CommonAPI documentation
docgendir=../doc-gen
iconsdir=../../images/icons
imagedir=../../images
datestring=$(shell date "+%m-%d-%y")
verification_project_path=../org.genivi.commonapi.core.verification
commonapi_version=2.1.5

all: doc

doc: spec_html spec_pdf tut_html tut_pdf test_html test_pdf

spec_html: docgendir
	mkdir -p $(docgendir)/spec/html
	asciidoc \
		-a revdate=$(datestring) \
		-a icons \
		-a iconsdir=$(iconsdir) \
		-a imagedir=$(imagedir) \
		-a version=$(commonapi_version) \
		-a cppstr=cpp \
		-a tabsize=4 \
		-o $(docgendir)/spec/html/CommonAPISpecification.html \
		CommonAPISpecification

spec_pdf: docgendir
	mkdir -p $(docgendir)/spec/pdf
	a2x \
		-a revdate=$(datestring) \
		-a icons \
		-a iconsdir=$(iconsdir) \
		-a imagedir=$(imagedir) \
		-a version=$(commonapi_version) \
		-a tabsize=4 \
		-f pdf \
		--dblatex-opts "-P latex.output.revhistory=0" \
		--icons \
		-D $(docgendir)/spec/pdf \
		CommonAPISpecification

tut_html: docgendir
	mkdir -p $(docgendir)/tutorial/html
	asciidoc -a revdate=$(datestring) \
				-a icons \
				-a iconsdir=$(iconsdir) \
				-a imagedir=$(imagedir) \
				-a version=$(commonapi_version) \
				-a cppstr=cpp \
				-a tabsize=4 \
				-b html \
				-o $(docgendir)/tutorial/html/Tutorial.html \
				Tutorial

tut_pdf: docgendir
	mkdir -p $(docgendir)/tutorial/pdf
	a2x \
		-a revdate=$(datestring) \
		-a icons \
		-a iconsdir=$(iconsdir) \
		-a imagedir=$(imagedir) \
		-a version=$(commonapi_version) \
		-a tabsize=4 \
		-f pdf \
		--dblatex-opts "-P latex.output.revhistory=0 -P imagedata.default.scale=maxwidth=10cm,maxheight=8cm" \
		--icons \
		-D $(docgendir)/tutorial/pdf \
		Tutorial

test_html: docgendir
	mkdir -p $(docgendir)/tests
	OUTPUT_DIRECTORY=$(docgendir)/tests \
	PROJECT_NAME=CommonAPITests \
	SRC_DIR='$(verification_project_path)/src ../doc/mainpagetests' \
	doxygen $(verification_project_path)/doxygen-test.cfg

test_pdf: test_html
	cd $(docgendir)/tests/latex; make > /dev/null
	mkdir -p $(docgendir)/tests/pdf
	cp $(docgendir)/tests/latex/refman.pdf $(docgendir)/tests/pdf/CommonAPITests.pdf
	rm -rf $(docgendir)/tests/latex

docgendir:
	mkdir -p $(docgendir)
	cp -rf images $(docgendir)/images

clean:
	rm -rf $(docgendir)
