doc:
	mkdir -p doc/html
	asciidoc -b html -o doc/html/README.html README
	asciidoc -b html -o doc/html/Tutorial.html Tutorial
	cp -a CommonAPI-Examples doc/html/.

clean:
	rm -rf doc/html/*
	rmdir doc/html
	rmdir doc
