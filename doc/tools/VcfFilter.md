### VcfFilter tool help
	VcfFilter (2018_10-35-g8d0245d)
	
	Filters a VCF based on the given criteria.
	
	Optional parameters:
	  -in <file>             Input VCF file. If unset, reads from STDIN.
	                         Default value: ''
	  -out <file>            Output VCF list. If unset, writes to STDOUT.
	                         Default value: ''
	  -reg <string>          Region of interest in BED format, or comma-separated list of region, e.g. 'chr1:454540-454678,chr2:473457-4734990'.
	                         Default value: ''
	  -variant_type <string> Filters by variant type. Possible types are: snp','indel','multi-allelic','other.
	                         Default value: ''
	  -id <string>           Filter by ID column (regular expression).
	                         Default value: ''
	  -qual <float>          Filter by QUAL column (minimum).
	                         Default value: '0'
	  -filter <string>       Filter by FILTER column (regular expression).
	                         Default value: ''
	  -filter_empty          Removes entries with non-empty FILTER column.
	                         Default value: 'false'
	  -info <string>         Filter by INFO column entries - use ';' as separator for several filters, e.g. 'DP > 5;AO > 2' (spaces are important).
	Valid operations are '>','>=','=','!=','<=','<','is','not','contains'.
	                         Default value: ''
	  -sample <string>       Filter by sample-specific entries - use ';' as separator for several filters, e.g. 'GT is 1/1' (spaces are important).
	Valid operations are '>','>=','=','!=','<=','<','is','not','contains'.
	                         Default value: ''
	  -sample_one_match      If sample_one_match is active samples are in OR mode. They will pass a filter once one or more of the sample passes the filters.
	                         Default value: 'false'
	
	Special parameters:
	  --help                 Shows this help and exits.
	  --version              Prints version and exits.
	  --changelog            Prints changeloge and exits.
	  --tdx                  Writes a Tool Definition Xml file. The file name is the application name with the suffix '.tdx'.
	
### VcfFilter changelog
	VcfFilter 2018_10-35-g8d0245d
	
	2018-10-31 Initial implementation.
[back to ngs-bits](https://github.com/imgag/ngs-bits)