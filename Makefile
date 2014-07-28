compile: build/pebble-my-data.pbw

run: build/pebble-my-data.pbw
	pebble install --logs

install: build/pebble-my-data.pbw
	pebble install

clean:
	pebble clean
	rm -f src/js/pebble-js-app.js

build/pebble-my-data.pbw: src/js/pebble-js-app.js src/pebble-my-data.c appinfo.json
	pebble build

src/js/pebble-js-app.js: src/js/pebble-js-app.src.js resources/configuration.html
	perl -pe 'BEGIN { local $$/; open $$fh,pop @ARGV or die $$!; $$f = <$$fh>; $$f =~ s/\047/\\\047/g; $$f =~ s/\n//g; } s/_HTMLMARKER_/$$f/g;' $^ | uglifyjs > $@

