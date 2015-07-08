#!/usr/bin/awk -f
BEGIN {
    FS="=";
    file=ARGV[1];
    xl=ENVIRON["ELIOT"];
    base=file;
    sub("[.]eliot", "", base);
    csource=file;
    sub("[.]eliot", ".c", csource);
    subdir=file;
    sub("/[^/]*$", "", subdir);
}
/\/\/ [A-Z]+=/ {
    name=substr($1, 4);
    value=$2;
    gsub("%x", xl, value);
    gsub("%f", file, value);
    gsub("%b", base, value);
    gsub("%c", csource, value);
    gsub("%d", subdir, value);
    print name "='" value "'";
}

