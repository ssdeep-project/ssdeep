clean:
	rm -f *~
	cp ../NEWS changes.txt
	man ../ssdeep.1 | col -bx > manpage.txt

publish:
	scp ssdeep.html jessekornblum@ssh.sf.net:/home/groups/s/ss/ssdeep/htdocs/

all: 
	scp changes.txt manpage.txt *.css *.html jessekornblum@ssh.sf.net:/home/groups/s/ss/ssdeep/htdocs/

