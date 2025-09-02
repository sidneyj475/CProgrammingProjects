projName=proj2
compileCmd=gcc -Wall -pedantic-errors $(projName).c -g -o $(projName).out

compile: $(projName).out

$(projName).out: $(projName).c
	$(compileCmd)

run: $(projName).out
	./$(projName).out

clean:
	rm $(projName).out

