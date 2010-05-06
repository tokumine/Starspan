//
// STARSpan project
// Carlos A. Rueda
// starspan2 
// $Id: starspan2.cc,v 1.43 2008-05-03 01:29:40 crueda Exp $
//

#include <geos/version.h>
#if GEOS_VERSION_MAJOR < 3
	#include <geos.h>
#else
	#include <geos/unload.h>
	using namespace geos::io;   // for Unload
#endif


#include "starspan.h"           
#include "traverser.h"           

#include <cstdlib>
#include <ctime>


static bool show_dev_options = false;

GlobalOptions globalOptions;

// prints a help message
static void usage(const char* msg) {
	if ( msg ) { 
		fprintf(stderr, "starspan: %s\n", msg); 
		fprintf(stderr, "Type `starspan --help' for help\n");
		exit(1);
	}
	// header:
	fprintf(stdout, 
		"\n"
		"CSTARS starspan %s (%s %s)\n"
		"\n"
		, STARSPAN_VERSION, __DATE__, __TIME__
	);
	
	// developer options:
	if ( show_dev_options ) {
		fprintf(stdout, 
		"      --dump_geometries <filename>\n"
		"      --jtstest <filename>\n"
		"      --ppoly \n"
		"      --srs <srs>\n"
		);
	}
	else {	
		// main body:
		fprintf(stdout, 
		"USAGE:\n"
		"  starspan <inputs/commands/options>...\n"
		"\n"
		"      --vector <filename>\n"
		"      --layer <layername>     (optional; defaults to first layer in vector dataset)\n"
		"      --raster {<filenames>... | @fieldname}\n"
		"      --mask <filenames>... \n"
		"      --speclib <filename>\n"
		"      --update-csv <filename>\n"
		"      --raster_directory <directory>\n"
		"      --csv <name>\n"
		"      --envi <name>\n"
		"      --envisl <name> \n"
		"      --stats outfile.csv {avg|mode|stdev|min|max|sum|median|nulls}...\n"
		"      --count-by-class outfile.csv \n"
		"      --calbase <link> <filename> [<stats>...]\n"
		"      --in   \n"
		"      --mini_rasters <prefix> \n"
		"      --mini_raster_strip <filename> [<shpname>]\n"
		"      --mini_raster_parity {even | odd | @<field>} \n"
		"      --separation <num-pixels> \n"
		"\n"
		"      --duplicate_pixel <mode> ...\n"
		"             <mode>: distance | direction <angle>\n"
		"      --fields <field1> <field2> ... <fieldn>\n"
		"      --pixprop <minimum-pixel-proportion>\n"
		"      --noColRow \n"
		"      --noXY\n"
		"      --sql <statement>\n"
		"      --where <condition>\n"
		"      --dialect <string>\n"
		"      --fid <FID>\n"
		"      --skip_invalid_polys \n"
		"      --nodata <value> \n"
		"      --buffer <distance> [<quadrantSegments>] \n"
		"      --box <width> [<height>] \n"
		"      --RID {file | path | none}\n"
		"      --delimiter <separator>\n"
		"      --progress [<value>] \n"
		"      --show-fields \n"
		"      --report \n"
		"      --verbose \n"
		"      --version\n"
		);
	}
	
	// footer:
	fprintf(stdout, 
		"\n"
		"Additional information at http://starspan.casil.ucdavis.edu\n"
		"\n"
	);
	
	exit(0);
}

// prints a help message
static void usage_string(string& msg) {
	const char* m = msg.c_str();
	usage(m);
}


///////////////////////////////////////////////////////////////
// main test program
int main(int argc, char ** argv) {
    
	globalOptions.use_pixpolys = false;
	globalOptions.skip_invalid_polys = false;
	globalOptions.pix_prop = -1.0;
	globalOptions.FID = -1;
	globalOptions.verbose = false;
	globalOptions.progress = false;
	globalOptions.progress_perc = 1;
	globalOptions.noColRow = false;
	globalOptions.noXY = false;
	globalOptions.only_in_feature = false;
	globalOptions.RID = "file";
	globalOptions.report_summary = true;
	globalOptions.nodata = 0.0;
	globalOptions.bufferParams.given = false;
	globalOptions.bufferParams.quadrantSegments = "1";
	globalOptions.boxParams.given = false;
	globalOptions.mini_raster_parity = "";
	globalOptions.mini_raster_separation = 0;
	globalOptions.delimiter = ",";
    

	if ( use_grass(&argc, argv) ) {
		return starspan_grass(argc, argv);
	}
	
	if ( argc == 1 ) {
		usage(NULL);
	}
	
    RasterizeParams rasterizeParams;
	

	bool report_elapsed_time = false;
	bool do_report = false;
	bool show_fields = false;
	const char*  envi_name = NULL;
	bool envi_image = true;
	vector<const char*>* select_fields = NULL;
	const char*  csv_name = NULL;
	const char*  stats_name = NULL;
	vector<const char*> select_stats;
	const char*  count_by_class_name = NULL;
	const char*  update_csv_name = NULL;
	const char*  mini_prefix = NULL;
	const char*  mini_srs = NULL;
	const char* mini_raster_strip_filename = NULL;
	const char* mini_raster_strip_shpfilename = NULL;
	const char* jtstest_filename = NULL;
	
	const char* vector_filename = 0;
	const char* vector_layername = 0;
	int vector_layernum = 0;
	vector<const char*> raster_filenames;
	
	const char* speclib_filename = 0;
	const char* callink_name = 0;
	const char* calbase_filename = 0;
	
	const char* raster_field_name = 0;
	const char* raster_directory = 0;
    
	vector<const char*> mask_filenames;
    vector<const char*> *masks = 0;
	const char* mask_directory = 0;

	const char* dump_geometries_filename = NULL;
	
	
	// ---------------------------------------------------------------------
	// collect arguments
	//
	for ( int i = 1; i < argc; i++ ) {
		
		//
		// INPUTS:
		//
		if ( 0==strcmp("--vector", argv[i]) ) {
			if ( ++i == argc || argv[i][0] == '-' )
				usage("--vector: which vector dataset?");
			if ( vector_filename )
				usage("--vector specified twice");
			vector_filename = argv[i];
		}
		
		else if ( 0==strcmp("--layer", argv[i]) ) {
			if ( ++i == argc || argv[i][0] == '-' )
				usage("--layer: which layer within the vector dataset?");
			if ( vector_layername )
				usage("--layer specified twice");
			vector_layername = argv[i];
		}
		
		else if ( 0==strcmp("--raster", argv[i]) ) {
			while ( ++i < argc && argv[i][0] != '-' ) {
				const char* raster_filename = argv[i];
				raster_filenames.push_back(raster_filename);
				// check for field indication:
				if ( raster_filename[0] == '@' )
					raster_field_name = raster_filename + 1;
				
				if ( raster_field_name && raster_filenames.size() > 1 ) 
					usage("--raster: only one element when indicating a @field");
			}
			if ( raster_filenames.size() == 0 )
				usage("--raster: missing argument(s)");
			if ( i < argc && argv[i][0] == '-' ) 
				--i;
		}
		else if ( 0==strcmp("--raster_directory", argv[i]) ) {
			if ( ++i == argc || argv[i][0] == '-' )
				usage("--raster_directory: missing argument");
			raster_directory = argv[i];
		}
		else if ( 0==strcmp("--mask", argv[i]) ) {
			while ( ++i < argc && argv[i][0] != '-' ) {
				const char* mask_filename = argv[i];
				mask_filenames.push_back(mask_filename);
				// check for field indication:
				if ( mask_filename[0] == '@' ) {
					usage("--mask: @field specification not implemented");
				}
			}
			if ( mask_filenames.size() == 0 ) {
				usage("--mask: missing argument(s)");
            }
			if ( i < argc && argv[i][0] == '-' ) { 
				--i;
            }
		}
		else if ( 0==strcmp("--mask_directory", argv[i]) ) {
			if ( ++i == argc || argv[i][0] == '-' ) {
				usage("--raster_mask_directory: missing argument");
            }
			mask_directory = argv[i];
		}
		else if ( 0==strcmp("--duplicate_pixel", argv[i]) ) {
			while ( ++i < argc && argv[i][0] != '-' ) {
				string dup_code = argv[i];
				double dup_arg = 0;
				if ( dup_code == "direction" ) {
					// need angle argument:
					if ( ++i < argc && argv[i][0] != '-' ) {
						dup_arg = atof(argv[i]);
					}
					else {
						usage("direction: missing angle parameter");
					}
                    globalOptions.dupPixelModes.push_back(DupPixelMode(dup_code, dup_arg));
				}
				else if ( dup_code == "distance" ) {
                    globalOptions.dupPixelModes.push_back(DupPixelMode(dup_code));
				}
				else {
					string msg = string("--duplicate_pixel: unrecognized mode: ") +dup_code;
					usage_string(msg);
				}
			}
			if ( globalOptions.dupPixelModes.size() == 0 ) {
				usage("--duplicate_pixel: specify at least one mode");
			}
			
			if ( i < argc && argv[i][0] == '-' ) { 
				--i;
			}
		}
		
		else if ( 0==strcmp("--speclib", argv[i]) ) {
			if ( ++i == argc || argv[i][0] == '-' )
				usage("--speclib: which CSV file?");
			if ( speclib_filename )
				usage("--speclib specified twice");
			speclib_filename = argv[i];
		}
		
		else if ( 0==strcmp("--update-csv", argv[i]) ) {
			if ( ++i == argc || argv[i][0] == '-' )
				usage("--update-csv: which CSV file?");
			if ( update_csv_name )
				usage("--update-csv specified twice");
			update_csv_name = argv[i];
		}
		
		//
		// COMMANDS
		//
		else if ( 0==strcmp("--calbase", argv[i]) ) {
			if ( ++i == argc || argv[i][0] == '-' )
				usage("--calbase: which field to use as link?");
			callink_name = argv[i];
			if ( ++i == argc || argv[i][0] == '-' )
				usage("--calbase: which output file name?");
			calbase_filename = argv[i];
			while ( ++i < argc && argv[i][0] != '-' ) {
				select_stats.push_back(argv[i]);
			}
			if ( select_stats.size() == 0 )
				select_stats.push_back("avg");
			if ( i < argc && argv[i][0] == '-' ) 
				--i;
		}
		
		else if ( 0==strcmp("--csv", argv[i]) ) {
			if ( ++i == argc || argv[i][0] == '-' )
				usage("--csv: which name?");
			csv_name = argv[i];
		}
		
		else if ( 0==strcmp("--stats", argv[i]) ) {
			if ( ++i == argc || argv[i][0] == '-' )
				usage("--stats: which output name?");
			stats_name = argv[i];
			while ( ++i < argc && argv[i][0] != '-' ) {
				select_stats.push_back(argv[i]);
			}
			if ( select_stats.size() == 0 )
				usage("--stats: which statistics?");
			if ( i < argc && argv[i][0] == '-' ) 
				--i;
		}

		else if ( 0==strcmp("--count-by-class", argv[i]) ) {
			if ( ++i == argc || argv[i][0] == '-' )
				usage("--count-by-class: which output name?");
			count_by_class_name = argv[i];
		}

		else if ( 0==strcmp("--mini_raster_strip", argv[i]) ) {
			if ( ++i == argc || argv[i][0] == '-' )
				usage("--mini_raster_strip: filename?");
			mini_raster_strip_filename = argv[i];
            if ( 1 + i < argc && argv[1 + i][0] != '-' ) {
                mini_raster_strip_shpfilename = argv[++i];
            }
		}
		
		else if ( 0==strcmp("--mini_rasters", argv[i]) ) {
			if ( ++i == argc || argv[i][0] == '-' )
				usage("--mini_rasters: which prefix?");
			mini_prefix = argv[i];
		}
		
		else if ( 0==strcmp("--rasterize", argv[i]) ) {
			if ( ++i == argc || argv[i][0] == '-' ) {
				usage("--rasterize: output raster name?");
            }
			rasterizeParams.outRaster_filename = argv[i];
            rasterizeParams.fillNoData = true;
            // TODO: accept other parameters
		}
        
		else if ( 0==strcmp("--envi", argv[i]) || 0==strcmp("--envisl", argv[i]) ) {
			envi_image = 0==strcmp("--envi", argv[i]);
			if ( ++i == argc || argv[i][0] == '-' )
				usage("--envi, --envisl: which base name?");
			envi_name = argv[i];
		}
		
		else if ( 0==strcmp("--dump_geometries", argv[i]) ) {
			if ( ++i == argc || argv[i][0] == '-' )
				usage("--dump_geometries: which name?");
			dump_geometries_filename = argv[i];
		}
		
		else if ( 0==strcmp("--jtstest", argv[i]) ) {
			if ( ++i == argc || argv[i][0] == '-' )
				usage("--jtstest: which JTS test file name?");
			jtstest_filename = argv[i];
		}
		else if ( 0==strcmp("--report", argv[i]) ) {
			do_report = true;
		}
		else if ( 0==strcmp("--show-fields", argv[i]) ) {
			show_fields = true;
		}
		
		//
		// OPTIONS
		//                                                        
		else if ( 0==strcmp("--pixprop", argv[i]) ) {
			if ( ++i == argc || argv[i][0] == '-' )
				usage("--pixprop: pixel proportion?");
			globalOptions.pix_prop = atof(argv[i]);
			if ( globalOptions.pix_prop < 0.0 || globalOptions.pix_prop > 1.0 )
				usage("invalid pixel proportion");
		}
		
		else if ( 0==strcmp("--nodata", argv[i]) ) {
			if ( ++i == argc || strncmp(argv[i], "--", 2) == 0 )
				usage("--nodata: value?");
			globalOptions.nodata = atof(argv[i]);
		}
		
		else if ( 0==strcmp("--buffer", argv[i]) ) {
			if ( ++i == argc || strncmp(argv[i], "--", 2) == 0 )
				usage("--buffer: distance?");
			globalOptions.bufferParams.distance = argv[i];
			if ( i+1 < argc && strncmp(argv[i+1], "--", 2) != 0 )
				globalOptions.bufferParams.quadrantSegments = argv[++i];
			globalOptions.bufferParams.given = true;				
		}
		
		else if ( 0==strcmp("--box", argv[i]) ) {
			if ( ++i == argc || strncmp(argv[i], "--", 2) == 0 ) {
				usage("--box: missing argument");
            }
            string bw = argv[i];
            string bh = bw;
            if ( 1 + i < argc && strncmp(argv[1 + i], "--", 2) != 0 ) {
                bh = argv[++i];
            }
            globalOptions.boxParams.width  = atof(bw.c_str());
            globalOptions.boxParams.height = atof(bh.c_str());
            globalOptions.boxParams.given = true;
		}
		
		else if ( 0==strcmp("--mini_raster_parity", argv[i]) ) {
			if ( ++i == argc || strncmp(argv[i], "--", 2) == 0 )
				usage("--mini_raster_parity: even, odd, or @<field>?");
			globalOptions.mini_raster_parity = argv[i];
		}
		
		else if ( 0==strcmp("--separation", argv[i]) ) {
			if ( ++i == argc || strncmp(argv[i], "--", 2) == 0 )
				usage("--separation: number of pixels?");
			globalOptions.mini_raster_separation = atoi(argv[i]);
			if ( globalOptions.mini_raster_separation < 0 )
				usage("--separation: invalid number of pixels");
		}
		
		else if ( 0==strcmp("--fid", argv[i]) ) {
			if ( ++i == argc || argv[i][0] == '-' )
				usage("--fid: desired FID?");
			globalOptions.FID = atol(argv[i]);
			if ( globalOptions.FID < 0 )
				usage("invalid FID");
		}
		
		else if ( 0==strcmp("--sql", argv[i]) ) {
			if ( ++i == argc || argv[i][0] == '-' )
				usage("--sql: missing statement");
			globalOptions.vSelParams.sql = argv[i];
		}
		
		else if ( 0==strcmp("--where", argv[i]) ) {
			if ( ++i == argc || argv[i][0] == '-' )
				usage("--where: missing condition");
			globalOptions.vSelParams.where = argv[i];
		}
		
		else if ( 0==strcmp("--dialect", argv[i]) ) {
			if ( ++i == argc || argv[i][0] == '-' )
				usage("--dialect: missing string");
			globalOptions.vSelParams.dialect = argv[i];
		}
		
		else if ( 0==strcmp("--noColRow", argv[i]) ) {
			globalOptions.noColRow = true;
		}
		
		else if ( 0==strcmp("--noXY", argv[i]) ) {
			globalOptions.noXY = true;
		}
		
		else if ( 0==strcmp("--ppoly", argv[i]) ) {
			globalOptions.use_pixpolys = true;
		}
		
		else if ( 0==strcmp("--skip_invalid_polys", argv[i]) ) {
			globalOptions.skip_invalid_polys = true;
		}
		
		else if ( 0==strcmp("--fields", argv[i]) ) {
			if ( !select_fields ) {
				select_fields = new vector<const char*>();
			}
			// the special name "none" will indicate not fields at all:
			bool none = false;
			while ( ++i < argc && argv[i][0] != '-' ) {
				const char* str = argv[i];
				none = none || 0==strcmp(str, "none");
				if ( !none ) {
					select_fields->push_back(str);
				}
			}
			if ( none ) {
				select_fields->clear();
			}
			if ( i < argc && argv[i][0] == '-' ) 
				--i;
		}

		else if ( 0==strcmp("--in", argv[i]) ) {
			globalOptions.only_in_feature = true;
		}
		
		else if ( 0==strcmp("--RID", argv[i]) ) {
			if ( ++i == argc || argv[i][0] == '-' ) {
				usage("--RID: what raster ID?");
			}
			globalOptions.RID = argv[i];
			if ( globalOptions.RID != "file"
			&&   globalOptions.RID != "path"
			&&   globalOptions.RID != "none" ) {
				usage("--RID: expecting one of: file, path, none");
			}
		}
		
		else if ( 0==strcmp("--delimiter", argv[i]) ) {
			if ( ++i == argc || strncmp(argv[i], "--", 2) == 0 ) {
				usage("--delimiter: what separator?");
			}
			globalOptions.delimiter = argv[i];
		}
		
		else if ( 0==strcmp("--progress", argv[i]) ) {
			if ( i+1 < argc && argv[i+1][0] != '-' )
				globalOptions.progress_perc = atof(argv[++i]);
			globalOptions.progress = true;
		}
		
		else if ( 0==strcmp("--verbose", argv[i]) ) {
			globalOptions.verbose = true;
			report_elapsed_time = true;
		}
		
		else if ( 0==strcmp("--srs", argv[i]) ) {
			if ( ++i == argc || argv[i][0] == '-' )
				usage("--srs: which srs?");
			mini_srs = argv[i];
		}

		// HELP: --help or --help-dev
		else if ( 0==strncmp("--help", argv[i], 6) ) {
			show_dev_options = 0==strcmp("--help-dev", argv[i]);
			usage(NULL);
		}
		// version
		else if ( 0==strcmp("--version", argv[i]) ) {
			fprintf(stdout, "starspan %s\n", STARSPAN_VERSION);
			exit(0);
		}
		else {
			usage("invalid arguments");
		}
	}
	// ---------------------------------------------------------------------
	
    
    if ( globalOptions.bufferParams.given && globalOptions.boxParams.given ) {
        usage("Only one of --buffer/--box may be given");
    }
    
	time_t time_start = time(NULL);
	
	// module initialization
	Raster::init();
	Vector::init();

	int res = 0;
	
	Vector* vect = 0;
	
	// preliminary checks:
	
	if ( vector_filename ) {
		vect = Vector::open(vector_filename);
		if ( !vect ) {
			fprintf(stderr, "Cannot open %s\n", vector_filename);
			res = 1;
			goto end;
		}  

        if ( vector_layername && globalOptions.vSelParams.sql.length() > 0 ) {
            cout<< "Warning: --layer option ignored when --sql option is given" <<endl;
        }
        
		// 
		// get the layer number for the given layer name 
		// if not specified or there is only a single layer, use the first layer 
		//
		if ( vector_layername && vect->getLayerCount() != 1 ) { 
			for ( unsigned i = 0; i < vect->getLayerCount(); i++ ) {    
				if ( 0 == strcmp(vector_layername, 
						 vect->getLayer(i)->GetLayerDefn()->GetName()) ) {
					vector_layernum = i;
				}
			}
		}
		else {
		       vector_layernum = 0; 
		}
	}

	if ( raster_field_name && !csv_name ) {
		usage("--csv command expected (as this is the only command\n"
			"that currently processes the --raster @fieldname specification)"
		);
	}
	if ( raster_directory ) {
        if ( raster_filenames.size() > 1 ) {
            // TODO
            //expandFileNames(raster_directory, raster_filenames);
        }
        else if ( !raster_field_name  ) {
            usage("--raster_directory: --raster required");
        }
	}
	
	
    if ( mask_filenames.size() > 0 ) {
        size_t noPairs = min(raster_filenames.size(), mask_filenames.size());
        raster_filenames.resize(noPairs);
        mask_filenames.resize(noPairs);
        masks = &mask_filenames;
        
        if ( globalOptions.verbose ) {
            cout<< "raster-mask pairs given:" << endl;
            for ( size_t i = 0; i < noPairs; i++ ) {
                cout<< "\t" <<raster_filenames[i]<< endl
                    << "\t" <<mask_filenames[i]<< endl<<endl;
            }
        }
    }
    
    if ( starspan_validate_rasters_and_masks(vect, vector_layernum, raster_filenames, masks) ) {
        usage("invalid inputs; check error messages");
    }        
    

	if ( globalOptions.dupPixelModes.size() > 0 ) {
		if ( globalOptions.verbose ) {
			cout<< "--duplicate_pixel modes given:" << endl;
			for (int k = 0, count = globalOptions.dupPixelModes.size(); k < count; k++ ) {
				cout<< "\t" <<globalOptions.dupPixelModes[k].toString() << endl;
			}
		}
		
		if ( !csv_name && !mini_prefix && !mini_raster_strip_filename ) {
			usage("--duplicate_pixel work with: --csv, --mini_rasters, or mini_raster_strip");
		}
	}
	
	//
	// dispatch commands with special processing:
	//
	if ( csv_name ) { 
		if ( !vect ) {
			usage("--csv expects a vector input (use --vector)");
		}
		if ( raster_field_name ) {
			if ( globalOptions.verbose ) {
				cout<< "--csv: using field for raster input: " <<raster_field_name<< endl; 
			}
			if ( !raster_directory )
				raster_directory = ".";
			
			res = starspan_csv_raster_field(
				vect,  
				raster_field_name,
				raster_directory,
				select_fields, 
				csv_name,
				vector_layernum
			);
		}
		else if ( globalOptions.dupPixelModes.size() > 0 ) {
            res = starspan_csv2(
                vect,
                raster_filenames,
                masks,
                select_fields,
                vector_layernum,
                globalOptions.dupPixelModes,
                csv_name
            );
		}
		else if ( raster_filenames.size() > 0 ) {
			res = starspan_csv(
				vect,  
				raster_filenames,
				select_fields, 
				csv_name,
				vector_layernum
			);
		}
		else {
			usage("--csv expects at least a raster input (use --raster)");
		}
	}
    
    else if ( mini_prefix && globalOptions.dupPixelModes.size() > 0 ) {
        res = starspan_miniraster2(
            vect,  
            raster_filenames,
            masks,
            select_fields, 
            vector_layernum,
            globalOptions.dupPixelModes,
            mini_prefix,
            mini_srs
        );
    }

    else if ( mini_raster_strip_filename && globalOptions.dupPixelModes.size() > 0 ) {
        res = starspan_minirasterstrip2(
            vect,  
            raster_filenames,
            masks,
            select_fields, 
            vector_layernum,
            globalOptions.dupPixelModes,
            mini_raster_strip_filename, mini_raster_strip_shpfilename
        );
    }

	
	else if ( stats_name ) {
		//Observer* obs = starspan_getStatsObserver(tr, select_stats, select_fields, stats_name);
		//if ( obs )
		//	tr.addObserver(obs);
		
		res = starspan_stats(
			vect,  
			raster_filenames,     
			select_stats,
			select_fields, 
			stats_name,
			vector_layernum
		);
		
	}
	
	else if ( calbase_filename ) {
		if ( !vect ) {
			usage("--calbase expects a vector input (use --vector)");
		}
		if ( raster_filenames.size() == 0 ) {
			usage("--calbase requires at least a raster input (use --raster)");
		}
		if ( !speclib_filename ) {
			usage("--calbase expects a speclib input (use --speclib)");
		}
		res = starspan_tuct_2(
			vect,  
			raster_filenames,
			speclib_filename,
			callink_name,
			select_stats,
			calbase_filename
		);
	}
	else if ( update_csv_name ) {
		if ( !csv_name ) {
			usage("--update-csv works with --csv. Please specify an existing CSV");
		}
		if ( vect ) {
			usage("--update-csv does not expect a vector input");
		}
		if ( raster_filenames.size() == 0 ) {
			usage("--update-csv requires at least a raster input");
		}
		res = starspan_update_csv(update_csv_name, raster_filenames, csv_name);
	}
	else if ( do_report ) {
		bool done = vect || raster_filenames.size() > 0;
		if ( vect ) {
			starspan_report_vector(vect);
		}
		for ( unsigned i = 0; i < raster_filenames.size(); i++ ) {
			Raster* rast = new Raster(raster_filenames[i]);
			starspan_report_raster(rast);
			delete rast;
		}

		if ( !done ) {
			usage("--report: Please give at least one input file to report\n");
		}
	}
	else {
		//
		// Dispatch commands with direct traverser-based mechanism.
		//
		
		// the traverser object	
		Traverser tr;
	
		if ( globalOptions.pix_prop >= 0.0 )
			tr.setPixelProportion(globalOptions.pix_prop);
	
        tr.setVectorSelectionParams(globalOptions.vSelParams);
		
		if ( globalOptions.FID >= 0 )
			tr.setDesiredFID(globalOptions.FID);
		
		tr.setVerbose(globalOptions.verbose);
		if ( globalOptions.progress )
			tr.setProgress(globalOptions.progress_perc, cout);
	
		tr.setSkipInvalidPolygons(globalOptions.skip_invalid_polys);

		if ( vect ) {
			tr.setVector(vect);
			tr.setLayerNum(vector_layernum);
		}
		
		for ( unsigned i = 0; i < raster_filenames.size(); i++ ) {    
			tr.addRaster(new Raster(raster_filenames[i]));
		}
	
		if ( globalOptions.bufferParams.given ) {
			if ( globalOptions.verbose ) {
				cout<< "Using buffer parameters:"
				    << "\n\tdistance = " <<globalOptions.bufferParams.distance
				    << "\n\tquadrantSegments = " <<globalOptions.bufferParams.quadrantSegments
					<<endl;
				;
			}
			tr.setBufferParameters(globalOptions.bufferParams);
		}
		else if ( globalOptions.boxParams.given ) {
			if ( globalOptions.verbose ) {
				cout<< "Using fixed box parameters:"
				    << "\n\twidth = " <<globalOptions.boxParams.width
				    << "\n\theight = " <<globalOptions.boxParams.height
					<<endl;
				;
			}
			tr.setBoxParameters(globalOptions.boxParams);
		}
			
		if ( csv_name || envi_name || mini_prefix || mini_raster_strip_filename || jtstest_filename) { 
			if ( tr.getNumRasters() == 0 || !tr.getVector() ) {
				usage("Specified output option requires both a raster and a vector to proceed\n");
			}
		}
	
	
		//	
		// COMMANDS
		//
		
		if ( count_by_class_name ) {
			Observer* obs = starspan_getCountByClassObserver(tr, count_by_class_name);
			if ( obs )
				tr.addObserver(obs);
		}
		
		if ( envi_name ) {
			Observer* obs = starspan_gen_envisl(tr, select_fields, envi_name, envi_image);
			if ( obs )
				tr.addObserver(obs);
		}
		
		if ( dump_geometries_filename ) {
			if ( tr.getNumRasters() == 0 && tr.getVector() && globalOptions.FID >= 0 ) {
				dumpFeature(tr.getVector(), globalOptions.FID, dump_geometries_filename);
			}
			else {
				Observer* obs = starspan_dump(tr, dump_geometries_filename);
				if ( obs )
					tr.addObserver(obs);
			}
		}
	
		if ( jtstest_filename ) {
			Observer* obs = starspan_jtstest(tr, jtstest_filename);
			if ( obs )
				tr.addObserver(obs);
		}
	
		if ( mini_raster_strip_filename ) {
			Observer* obs = starspan_getMiniRasterStripObserver(
                tr, mini_raster_strip_filename, mini_raster_strip_shpfilename
            );
			if ( obs ) {
				tr.addObserver(obs);
            }
		}
	
		if ( mini_prefix ) {
			Observer* obs = starspan_getMiniRasterObserver(mini_prefix, mini_srs);
			if ( obs ) {
				tr.addObserver(obs);
            }
		}
	
		if ( rasterizeParams.outRaster_filename ) {
            GDALDataset* ds = tr.getRaster(0)->getDataset();
            rasterizeParams.projection = ds->GetProjectionRef();
            ds->GetGeoTransform(rasterizeParams.geoTransform);
			Observer* obs = starspan_getRasterizeObserver(&rasterizeParams);
			if ( obs ) {
				tr.addObserver(obs);
            }
		}
	

		// the following don't use Observer scheme;  
		// just call corresponding functions:	
		if ( show_fields ) {
			if ( !tr.getVector() ) {
				usage("--show-fields: Please provide the vector datasource\n");
			}
			tr.getVector()->showFields(stdout);
		}
		
		//
		// now observer-based processing:
		//
		
		if ( tr.getNumObservers() > 0 ) {
			// let's get to work!	
			tr.traverse();
	
			if ( globalOptions.report_summary ) {
				tr.reportSummary();
			}
			
			// release observers:
			tr.releaseObservers();
		}
		
		// release data input objects:
        // Note that we get the rasters from the traverser, while
        // the single vector is directly deleted below:
		for ( int i = 0; i < tr.getNumRasters(); i++ ) {
			delete tr.getRaster(i);
		}
	}
	
end:
	if ( vect ) {
		delete vect;
	}
	
	Vector::end();
	Raster::end();
	
	// more final cleanup:
	Unload::Release();	
	
	if ( report_elapsed_time ) {
		cout<< "Elapsed time: ";
		// report elapsed time:
		time_t secs = time(NULL) - time_start;
		if ( secs < 60 )
			cout<< secs << "s\n";
		else {
			time_t mins = secs / 60; 
			secs = secs % 60; 
			if ( mins < 60 ) 
				cout<< mins << "m:" << secs << "s\n";
			else {
				time_t hours = mins / 60; 
				mins = mins % 60; 
				if ( hours < 24 ) 
					cout<< hours << "h:" << mins << "m:" << secs << "s\n";
				else {
					time_t days = hours / 24; 
					hours = hours % 24; 
					cout<< days << "d:" <<hours << "h:" << mins << "m:" << secs << "s\n";
				}
			}
		}
	}

	return res;
}

