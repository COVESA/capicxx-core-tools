doc:
	mkdir -p doc/html
	asciidoc -b html -o doc/html/README.html README
	asciidoc -b html -o doc/html/Tutorial.html Tutorial

clean:
	rm doc/html/README.html doc/html/Tutorial.html
	rmdir doc/html
	rmdir doc
