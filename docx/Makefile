# Creating CommonAPI documentation
docgendir=../doc
htmldocgendir=$(docgendir)/html
pdfdocgendir=$(docgendir)/pdf
iconsdir=../images/icons
imagedir=../images
datestring=$(shell date "+%d.\ %b\ %Y")
verification_project_path=../org.genivi.commonapi.core.verification
commonapi_version=$(PROJECT_VERSION)

all: doc

doc: spec_html spec_pdf tut_html tut_pdf test_html test_pdf

spec_html: docgendir
	mkdir -p $(htmldocgendir)
	asciidoc \
		-a revdate=$(datestring) \
		-a icons \
		-a iconsdir=$(iconsdir) \
		-a imagedir=$(imagedir) \
		-a version=$(commonapi_version) \
		-a cppstr=cpp \
		-a tabsize=4 \
		-o $(htmldocgendir)/CommonAPISpecification.html \
		CommonAPISpecification

spec_pdf: docgendir
	mkdir -p $(pdfdocgendir)
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
		-D $(pdfdocgendir) \
		CommonAPISpecification

tut_html: docgendir
	mkdir -p $(htmldocgendir)
	asciidoc -a revdate=$(datestring) \
				-a icons \
				-a iconsdir=$(iconsdir) \
				-a imagedir=$(imagedir) \
				-a version=$(commonapi_version) \
				-a cppstr=cpp \
				-a tabsize=4 \
				-b html \
				-o $(htmldocgendir)/Tutorial.html \
				Tutorial

tut_pdf: docgendir
	mkdir -p $(pdfdocgendir)
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
		-D $(pdfdocgendir) \
		Tutorial

test_html: docgendir
	mkdir -p $(htmldocgendir)/tests
	OUTPUT_DIRECTORY=$(htmldocgendir)/tests \
	PROJECT_NAME=CommonAPITests \
	SRC_DIR='$(verification_project_path)/src ../docx/mainpagetests' \
	doxygen $(verification_project_path)/doxygen-test.cfg

test_pdf: test_html
	mkdir -p $(pdfdocgendir)/tests
	cd $(htmldocgendir)/tests/latex; make > /dev/null
	cd $(htmldocgendir)/tests
	cp $(htmldocgendir)/tests/latex/refman.pdf $(pdfdocgendir)/tests/CommonAPITests.pdf
	rm -rf $(htmldocgendir)/tests/latex

docgendir:
	mkdir -p $(docgendir)
	cp -rf images $(docgendir)/images
	cp index.html $(docgendir)

clean:
	rm -rf $(docgendir)
