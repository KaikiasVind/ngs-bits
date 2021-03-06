### PhenotypesToGenes tool help
	PhenotypesToGenes (2020_03-142-gee6a65cd)
	
	Converts a phenotype list to a list of matching genes.
	
	For each given HPO term, the genes associated with the term itself and the genes associated with any sub-term are returned.
	
	Optional parameters:
	  -in <string> Input file, containing one HPO term identifier per line, e.g. HP:0002066. Text after the identifier is ignored. If unset, reads from STDIN.
	               Default value: ''
	  -out <file>  Output TSV file with genes (column 1) and matched phenotypes (column 2). If unset, writes to STDOUT.
	               Default value: ''
	  -test        Uses the test database instead of on the production database.
	               Default value: 'false'
	
	Special parameters:
	  --help       Shows this help and exits.
	  --version    Prints version and exits.
	  --changelog  Prints changeloge and exits.
	  --tdx        Writes a Tool Definition Xml file. The file name is the application name with the suffix '.tdx'.
	
### PhenotypesToGenes changelog
	PhenotypesToGenes 2020_03-142-gee6a65cd
	
	2020-05-24 First version.
[back to ngs-bits](https://github.com/imgag/ngs-bits)