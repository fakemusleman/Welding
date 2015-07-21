// mmsp2vti.cpp
// Convert MMSP grid data to VTK image data format
// Questions/comments to gruberja@gmail.com (Jason Gruber)

#include"MMSP.hpp"
#include<zlib.h>
#include<sstream>
#include<vector>

double T0 = 1248;
double Tq = 2131;
double Teta = 0.5;//0.75;
double Tlambda =61000; // 0.5/(20/4000/610*1.0e6)
double Tk = 20; 
double Tv = 1.0e-3;
double Tepsilon = 1.0;
double repsilon = 100.0e-6;
double lambda = 30.0e-6;  //m
double physical_time = 2.24132;

int main(int argc, char* argv[]) {
	// command line error check
	if (argc < 4) {
		std::cout << "Usage: " << argv[0] << " [--help] infile [outfile]\n\n";
		exit(-1);
	}

	// help diagnostic
	if (std::string(argv[1]) == "--help") {
		std::cout << argv[0] << ": convert MMSP grid data to VTK image data format.\n";
		std::cout << "Usage: " << argv[0] << " [--help] infile [outfile]\n\n";
		std::cout << "Questions/comments to gruberja@gmail.com (Jason Gruber).\n\n";
		exit(0);
	}

	// file open error check
	std::ifstream input(argv[1]);
	if (!input) {
		std::cerr << "File input error: could not open " << argv[1] << ".\n\n";
		exit(-1);
	}

	// generate output file name
	std::stringstream filename;
	if (argc < 5)
		filename << std::string(argv[1]).substr(0, std::string(argv[1]).find_last_of(".")) << ".vti";
	else
		filename << argv[2];

	// file open error check
	std::ofstream output(filename.str().c_str());
	if (!output) {
		std::cerr << "File output error: could not open ";
		std::cerr << filename.str() << "." << std::endl;
		exit(-1);
	}

	// read data type
	std::string type;
	getline(input, type, '\n');

	// grid type error check
	if (type.substr(0, 4) != "grid") {
		std::cerr << "File input error: file does not contain grid data." << std::endl;
		exit(-1);
	}

	// parse data type
	bool bool_type = (type.find("bool") != std::string::npos);
	bool char_type = (type.find("char") != std::string::npos);
	bool unsigned_char_type = (type.find("unsigned char") != std::string::npos);
	bool int_type = (type.find("int") != std::string::npos);
	bool unsigned_int_type = (type.find("unsigned int") != std::string::npos);
	bool long_type = (type.find("long") != std::string::npos) && (type.find("unsigned long") == std::string::npos);
	bool unsigned_long_type = (type.find("unsigned long") != std::string::npos);
	bool short_type = (type.find("short") != std::string::npos);
	bool unsigned_short_type = (type.find("unsigned short") != std::string::npos);
	bool float_type = (type.find("float") != std::string::npos);
	bool double_type = (type.find("double") != std::string::npos);
	bool long_double_type = (type.find("long double") != std::string::npos);

	bool scalar_type = (type.find("scalar") != std::string::npos);
	bool vector_type = (type.find("vector") != std::string::npos);
	bool sparse_type = (type.find("sparse") != std::string::npos);

	if (not bool_type    and
	    not char_type    and  not unsigned_char_type   and
	    not int_type     and  not unsigned_int_type    and
	    not long_type    and  not unsigned_long_type   and
	    not short_type   and  not unsigned_short_type  and
	    not float_type   and
	    not double_type  and  not long_double_type) {
		std::cerr << "File input error: unknown grid data type." << std::endl;
		exit(-1);
	}

	// read grid dimension
	int dim;
	input >> dim;
	#ifdef DEBUG
	std::cout<<"Grid is "<<dim<<"-dimensional."<<std::endl;
	#endif

	// read number of fields
	int fields;
	input >> fields;
	#ifdef DEBUG
	std::cout<<"Grid has "<<fields<<" fields."<<std::endl;
	#endif

	// read grid sizes
	int g0[3] = {0, 0, 0};
	int g1[3] = {0, 0, 0};
	for (int i = 0; i < dim; i++)
		input >> g0[i] >> g1[i];
	#ifdef DEBUG
	std::cout<<"Grid edge is "<<g1[0] - g0[0]<<std::endl;
	#endif

	// read cell spacing
	float dx[3] = {1.0, 1.0, 1.0};
	for (int i = 0; i < dim; i++)
		input >> dx[i];
	#ifdef DEBUG
	std::cout<<"Grid spacing is "<<dx[0]<<std::endl;
	#endif

	// ignore trailing endlines
	input.ignore(10, '\n');


	// determine byte order
	std::string byte_order;
	if (0x01 & static_cast<int>(1)) byte_order = "LittleEndian";
	else byte_order = "BigEndian";
	#ifdef DEBUG
	std::cout<<"Grid is "<<byte_order<<std::endl;
	#endif

	// output header markup
	output << "<?xml version=\"1.0\"?>\n";
	output << "<VTKFile type=\"ImageData\" version=\"0.1\" byte_order=\"" << byte_order << "\">\n";

	if (dim == 1) {
		output << "  <ImageData WholeExtent=\"" << g0[0] << " " << g1[0] << " 0 0 0 0\"";
		output << "   Origin=\"0 0 0\" Spacing=\"" << dx[0] << " 1 1\">\n";
	}
	if (dim == 2) {
		output << "  <ImageData WholeExtent=\"" << g0[1] << " " << g1[1] << " " << g0[0] << " " << g1[0] << " 0 0\"";
		output << "   Origin=\"0 0 0\" Spacing=\"" << dx[1] << " " << dx[0] << " 1\">\n";
	}
	if (dim == 3) {
		output << "  <ImageData WholeExtent=\"" << g0[2] << " " << g1[2] << " " << g0[1] << " " << g1[1] << " " << g0[0] << " " << g1[0] << "\"";
		output << "   Origin=\"0 0 0\" Spacing=\"" << dx[2] << " " << dx[1] << " " << dx[0] << "\">\n";
	}


	// Estimate number of grains, for color randomization
	int est_grains = 10000;
	if (dim==2) est_grains=static_cast<int>(1.5*float((g1[0]-g0[0])*(g1[1]-g0[1]))/(M_PI*10.*10.)); // average grain is a disk of radius 10
	else if (dim==3) est_grains=static_cast<int>(1.5*float((g1[0]-g0[0])*(g1[1]-g0[1])*(g1[2]-g0[2]))/(4./3*M_PI*10.*10.*10.)); // Average grain is a sphere of radius 10 voxels
	#ifdef DEBUG
	std::cout<<"Grid contains approx. "<<est_grains<<" grains."<<std::endl;
	#endif
	std::vector<int> colors;
	for (unsigned int i=0; i<est_grains; i++)
		colors.push_back(rand() % est_grains);

	// read number of blocks
	int blocks;
	input.read(reinterpret_cast<char*>(&blocks), sizeof(blocks));

  std::vector<unsigned long>* grain_ids = new std::vector<unsigned long>;
  double r_fusion = atof(argv[argc-2]);
  double distance = atof(argv[argc-1]);

	for (int i = 0; i < blocks; i++) {
		// read block limits
		int lmin[3] = {0, 0, 0};
		int lmax[3] = {0, 0, 0};
		for (int j = 0; j < dim; j++) {
			input.read(reinterpret_cast<char*>(&lmin[j]), sizeof(lmin[j]));
			input.read(reinterpret_cast<char*>(&lmax[j]), sizeof(lmax[j]));
		}
		int blo[dim];
    int bhi[dim];
    // read boundary conditions
    for (int j = 0; j < dim; j++) {
      input.read(reinterpret_cast<char*>(&blo[j]), sizeof(blo[j]));
      input.read(reinterpret_cast<char*>(&bhi[j]), sizeof(bhi[j]));
    }

		// write header markup
		if (dim == 1) output << "    <Piece Extent=\"" << lmin[0] << " " << lmax[0] << " 0 0 0 0\">\n";
		if (dim == 2) output << "    <Piece Extent=\"" << lmin[1] << " " << lmax[1] << " " << lmin[0] << " " << lmax[0] << " 0 0\">\n";
		if (dim == 3) output << "    <Piece Extent=\"" << lmin[2] << " " << lmax[2] << " " << lmin[1] << " " << lmax[1] << " " << lmin[0] << " " << lmax[0] << "\">\n";

		// write cell data markup
		if (scalar_type) {
			output << "      <CellData>\n";
			output << "        <DataArray Name=\"scalar_data\"";
		}

		else if (vector_type) {
			output << "      <CellData>\n";
			output << "        <DataArray Name=\"vector_data\" NumberOfComponents=\"" << fields << "\"";
		}

		else if (sparse_type) {
			output << "      <CellData>\n";
			output << "        <DataArray Name=\"scalar_data\"";
		}

		else { /* built-in data types */
			output << "      <CellData>\n";
			output << "        <DataArray Name=\"scalar_data\"";
		}

		if (bool_type)
			output << " type=\"UInt8\" format=\"ascii\">\n";
		else if (char_type)
			output << " type=\"Int8\" format=\"ascii\">\n";
		else if (unsigned_char_type)
			output << " type=\"UInt8\" format=\"ascii\">\n";
		else if (int_type)
			output << " type=\"Int32\" format=\"ascii\">\n";
		else if (unsigned_int_type)
			output << " type=\"UInt32\" format=\"ascii\">\n";
		else if (long_type)
			output << " type=\"Int32\" format=\"ascii\">\n";
		else if (unsigned_long_type)
			output << " type=\"UInt32\" format=\"ascii\">\n";
		else if (short_type)
			output << " type=\"Int16\" format=\"ascii\">\n";
		else if (unsigned_short_type)
			output << " type=\"UInt16\" format=\"ascii\">\n";
		else if (float_type)
			output << " type=\"Float32\" format=\"ascii\">\n";
		else if (double_type)
			output << " type=\"Float64\" format=\"ascii\">\n";
		else if (long_double_type)
			output << " type=\"Float128\" format=\"ascii\">\n";

		// read grid data
		unsigned long size, rawSize;
		input.read(reinterpret_cast<char*>(&rawSize), sizeof(rawSize)); // read raw size
		input.read(reinterpret_cast<char*>(&size), sizeof(size)); // read compressed size
		char* compressed_buffer = new char[size];
		input.read(compressed_buffer, size);
		char* buffer = NULL;
		if (size!=rawSize) {
			// Decompress data
			buffer = new char[rawSize];
			int status;
			status = uncompress(reinterpret_cast<unsigned char*>(buffer), &rawSize, reinterpret_cast<unsigned char*>(compressed_buffer), size);
			switch( status ) {
			case Z_OK:
				break;
			case Z_MEM_ERROR:
				std::cerr << "Uncompress: out of memory." << std::endl;
				exit(1);
				break;
			case Z_BUF_ERROR:
				std::cerr << "Uncompress: output buffer wasn't large enough." << std::endl;
				exit(1);
				break;
			}
			delete [] compressed_buffer;
			compressed_buffer=NULL;
		} else {
			buffer = compressed_buffer;
			compressed_buffer=NULL;
		}

		// write grid data
		if (not vector_type and not sparse_type) {
			if (bool_type) {
				if (dim == 1) {
					MMSP::grid<1, MMSP::scalar<bool> > GRID(fields, lmin, lmax);
					GRID.from_buffer(buffer);
					for (int k = 0; k < MMSP::nodes(GRID); k++) output << GRID(k) << " ";
				} else if (dim == 2) {
					MMSP::grid<2, MMSP::scalar<bool> > GRID(fields, lmin, lmax);
					GRID.from_buffer(buffer);
					for (int k = 0; k < MMSP::nodes(GRID); k++) output << GRID(k) << " ";
				} else if (dim == 3) {
					MMSP::grid<3, MMSP::scalar<bool> > GRID(fields, lmin, lmax);
					GRID.from_buffer(buffer);
					for (int k = 0; k < MMSP::nodes(GRID); k++) output << GRID(k) << " ";
				}
			}
			if (char_type) {
				if (dim == 1) {
					MMSP::grid<1, MMSP::scalar<char> > GRID(fields, lmin, lmax);
					GRID.from_buffer(buffer);
					for (int k = 0; k < MMSP::nodes(GRID); k++) output << GRID(k) << " ";
				} else if (dim == 2) {
					MMSP::grid<2, MMSP::scalar<char> > GRID(fields, lmin, lmax);
					GRID.from_buffer(buffer);
					for (int k = 0; k < MMSP::nodes(GRID); k++) output << GRID(k) << " ";
				} else if (dim == 3) {
					MMSP::grid<3, MMSP::scalar<char> > GRID(fields, lmin, lmax);
					GRID.from_buffer(buffer);
					for (int k = 0; k < MMSP::nodes(GRID); k++) output << GRID(k) << " ";
				}
			}
			if (unsigned_char_type) {
				if (dim == 1) {
					MMSP::grid<1, MMSP::scalar<unsigned char> > GRID(fields, lmin, lmax);
					GRID.from_buffer(buffer);
					for (int k = 0; k < MMSP::nodes(GRID); k++) output << GRID(k) << " ";
				} else if (dim == 2) {
					MMSP::grid<2, MMSP::scalar<unsigned char> > GRID(fields, lmin, lmax);
					GRID.from_buffer(buffer);
					for (int k = 0; k < MMSP::nodes(GRID); k++) output << GRID(k) << " ";
				} else if (dim == 3) {
					MMSP::grid<3, MMSP::scalar<unsigned char> > GRID(fields, lmin, lmax);
					GRID.from_buffer(buffer);
					for (int k = 0; k < MMSP::nodes(GRID); k++) output << GRID(k) << " ";
				}
			}
			if (int_type) {
				if (dim == 1) {
					MMSP::grid<1, MMSP::scalar<int> > GRID(fields, lmin, lmax);
					GRID.from_buffer(buffer);
					for (int k = 0; k < MMSP::nodes(GRID); k++) output << colors[GRID(k)%est_grains] << " ";
				} else if (dim == 2) {
					MMSP::grid<2, MMSP::scalar<int> > GRID(fields, lmin, lmax);
					GRID.from_buffer(buffer);
					for (int k = 0; k < MMSP::nodes(GRID); k++) output << colors[GRID(k)%est_grains] << " ";
				} else if (dim == 3) {
					MMSP::grid<3, MMSP::scalar<int> > GRID(fields, lmin, lmax);
					GRID.from_buffer(buffer);
					for (int k = 0; k < MMSP::nodes(GRID); k++) output << colors[GRID(k)%est_grains] << " ";
				}
			}
			if (unsigned_int_type) {
				if (dim == 1) {
					MMSP::grid<1, MMSP::scalar<unsigned int> > GRID(fields, lmin, lmax);
					GRID.from_buffer(buffer);
					for (int k = 0; k < MMSP::nodes(GRID); k++) output << GRID(k) << " ";
				} else if (dim == 2) {
					MMSP::grid<2, MMSP::scalar<unsigned int> > GRID(fields, lmin, lmax);
					GRID.from_buffer(buffer);
					for (int k = 0; k < MMSP::nodes(GRID); k++) output << GRID(k) << " ";
				} else if (dim == 3) {
					MMSP::grid<3, MMSP::scalar<unsigned int> > GRID(fields, lmin, lmax);
					GRID.from_buffer(buffer);
					for (int k = 0; k < MMSP::nodes(GRID); k++) output << GRID(k) << " ";
				}
			}
			if (long_type) {
				if (dim == 1) {
					MMSP::grid<1, MMSP::scalar<long> > GRID(fields, lmin, lmax);
					GRID.from_buffer(buffer);
					for (int k = 0; k < MMSP::nodes(GRID); k++) output << GRID(k) << " ";
				} else if (dim == 2) {
					MMSP::grid<2, MMSP::scalar<long> > GRID(fields, lmin, lmax);
					GRID.from_buffer(buffer);
					for (int k = 0; k < MMSP::nodes(GRID); k++) output << GRID(k) << " ";
				} else if (dim == 3) {
					MMSP::grid<3, MMSP::scalar<long> > GRID(fields, lmin, lmax);
					GRID.from_buffer(buffer);
					for (int k = 0; k < MMSP::nodes(GRID); k++) output << GRID(k) << " ";
				}
			}
			if (unsigned_long_type) {
				if (dim == 1) {
					MMSP::grid<1, MMSP::scalar<unsigned long> > GRID(fields, lmin, lmax);
					GRID.from_buffer(buffer);
					for (long k = 0; k < MMSP::nodes(GRID); k++) output << GRID(k) << " ";
				} else if (dim == 2) {
					MMSP::grid<2, MMSP::scalar<unsigned long> > GRID(fields, lmin, lmax);
					GRID.from_buffer(buffer);
					for (long k = 0; k < MMSP::nodes(GRID); k++) output << GRID(k) << " ";
				} else if (dim == 3) {
					MMSP::grid<3, MMSP::scalar<unsigned long> > GRID(fields, lmin, lmax);
					GRID.from_buffer(buffer);
          MMSP::vector<int> coords (3,0);
          for(int codx=x0(GRID, 0); codx < x1(GRID, 0); codx++)  
            for(int cody=x0(GRID, 1); cody < x1(GRID, 1); cody++) 
              for(int codz=x0(GRID, 2); codz < x1(GRID, 2); codz++){
                coords[0] = codx;
                coords[1] = cody;
                coords[2] = codz;
                if(codz==x0(GRID, 2)){
                  double r = 0.0;
                  if(lambda*cody>Tv*physical_time)
                    r = sqrt(pow(lambda*codx,2) + pow(lambda*cody-Tv*physical_time, 2) + pow(lambda*codz,2)); 
                  else
                    r = sqrt(pow(lambda*codx,2) + pow(lambda*codz,2));
                  double temperature = T0 + Tq*Teta/2/3.1415926/Tk/r*exp(-Tlambda*Tv*(r-Tv*physical_time)); 
                  if(temperature>1878.0){
                    output << 0 << " ";
                  }else if(fabs(r_fusion+distance - r) < repsilon){
                    output << 0 << " ";
                    unsigned long site_id = GRID(coords);
                    bool InFlag = false;
                    if((*grain_ids).size() == 0)
                      (*grain_ids).push_back(site_id);
                    else{
                      for(int idx = 0; idx < (*grain_ids).size(); idx++){
                        if((*grain_ids)[idx] == site_id){
                          InFlag = true;
                          break;
                        }
                      }
                      if(InFlag == false)
                        (*grain_ids).push_back(site_id);
                    }
                  }//else if(fabs(r_fusion+distance - r)< repsilon){
                  else output << GRID(coords)%17 + 1<< " ";
                }//if(codz==x0(GRID, 2)){
              }//for(int codz=x0(GRID, 2); codz < x1(GRID, 2); codz
				}
			}
			if (short_type) {
				if (dim == 1) {
					MMSP::grid<1, MMSP::scalar<short> > GRID(fields, lmin, lmax);
					GRID.from_buffer(buffer);
					for (int k = 0; k < MMSP::nodes(GRID); k++) output << GRID(k) << " ";
				} else if (dim == 2) {
					MMSP::grid<2, MMSP::scalar<short> > GRID(fields, lmin, lmax);
					GRID.from_buffer(buffer);
					for (int k = 0; k < MMSP::nodes(GRID); k++) output << GRID(k) << " ";
				} else if (dim == 3) {
					MMSP::grid<3, MMSP::scalar<short> > GRID(fields, lmin, lmax);
					GRID.from_buffer(buffer);
					for (int k = 0; k < MMSP::nodes(GRID); k++) output << GRID(k) << " ";
				}
			}
			if (unsigned_short_type) {
				if (dim == 1) {
					MMSP::grid<1, MMSP::scalar<unsigned short> > GRID(fields, lmin, lmax);
					GRID.from_buffer(buffer);
					for (int k = 0; k < MMSP::nodes(GRID); k++) output << GRID(k) << " ";
				} else if (dim == 2) {
					MMSP::grid<2, MMSP::scalar<unsigned short> > GRID(fields, lmin, lmax);
					GRID.from_buffer(buffer);
					for (int k = 0; k < MMSP::nodes(GRID); k++) output << GRID(k) << " ";
				} else if (dim == 3) {
					MMSP::grid<3, MMSP::scalar<unsigned short> > GRID(fields, lmin, lmax);
					GRID.from_buffer(buffer);
					for (int k = 0; k < MMSP::nodes(GRID); k++) output << GRID(k) << " ";
				}
			}
			if (float_type) {
				if (dim == 1) {
					MMSP::grid<1, MMSP::scalar<float> > GRID(fields, lmin, lmax);
					GRID.from_buffer(buffer);
					for (int k = 0; k < MMSP::nodes(GRID); k++) output << GRID(k) << " ";
				} else if (dim == 2) {
					MMSP::grid<2, MMSP::scalar<float> > GRID(fields, lmin, lmax);
					GRID.from_buffer(buffer);
					for (int k = 0; k < MMSP::nodes(GRID); k++) output << GRID(k) << " ";
				} else if (dim == 3) {
					MMSP::grid<3, MMSP::scalar<float> > GRID(fields, lmin, lmax);
					GRID.from_buffer(buffer);
					for (int k = 0; k < MMSP::nodes(GRID); k++) output << GRID(k) << " ";
				}
			}
			if (double_type) {
				if (dim == 1) {
					MMSP::grid<1, MMSP::scalar<double> > GRID(fields, lmin, lmax);
					GRID.from_buffer(buffer);
					for (int k = 0; k < MMSP::nodes(GRID); k++) output << GRID(k) << " ";
				} else if (dim == 2) {
					MMSP::grid<2, MMSP::scalar<double> > GRID(fields, lmin, lmax);
					GRID.from_buffer(buffer);
					for (int k = 0; k < MMSP::nodes(GRID); k++) output << GRID(k) << " ";
				} else if (dim == 3) {
					MMSP::grid<3, MMSP::scalar<double> > GRID(fields, lmin, lmax);
					GRID.from_buffer(buffer);
					for (int k = 0; k < MMSP::nodes(GRID); k++) output << GRID(k) << " ";
				}
			}
			if (long_double_type) {
				if (dim == 1) {
					MMSP::grid<1, MMSP::scalar<long double> > GRID(fields, lmin, lmax);
					GRID.from_buffer(buffer);
					for (int k = 0; k < MMSP::nodes(GRID); k++) output << GRID(k) << " ";
				} else if (dim == 2) {
					MMSP::grid<2, MMSP::scalar<long double> > GRID(fields, lmin, lmax);
					GRID.from_buffer(buffer);
					for (int k = 0; k < MMSP::nodes(GRID); k++) output << GRID(k) << " ";
				} else if (dim == 3) {
					MMSP::grid<3, MMSP::scalar<long double> > GRID(fields, lmin, lmax);
					GRID.from_buffer(buffer);
					for (int k = 0; k < MMSP::nodes(GRID); k++) output << GRID(k) << " ";
				}
			}
		}

		else if (vector_type) {
			if (bool_type) {
				if (dim == 1) {
					MMSP::grid<1, MMSP::vector<bool> > GRID(fields, lmin, lmax);
					GRID.from_buffer(buffer);
					for (int k = 0; k < MMSP::nodes(GRID); k++)
						for (int h = 0; h < fields; h++) output << GRID(k)[h] << " ";
				} else if (dim == 2) {
					MMSP::grid<2, MMSP::vector<bool> > GRID(fields, lmin, lmax);
					GRID.from_buffer(buffer);
					for (int k = 0; k < MMSP::nodes(GRID); k++)
						for (int h = 0; h < fields; h++) output << GRID(k)[h] << " ";
				} else if (dim == 3) {
					MMSP::grid<3, MMSP::vector<bool> > GRID(fields, lmin, lmax);
					GRID.from_buffer(buffer);
					for (int k = 0; k < MMSP::nodes(GRID); k++)
						for (int h = 0; h < fields; h++) output << GRID(k)[h] << " ";
				}
			}
			if (char_type) {
				if (dim == 1) {
					MMSP::grid<1, MMSP::vector<char> > GRID(fields, lmin, lmax);
					GRID.from_buffer(buffer);
					for (int k = 0; k < MMSP::nodes(GRID); k++)
						for (int h = 0; h < fields; h++) output << GRID(k)[h] << " ";
				} else if (dim == 2) {
					MMSP::grid<2, MMSP::vector<char> > GRID(fields, lmin, lmax);
					GRID.from_buffer(buffer);
					for (int k = 0; k < MMSP::nodes(GRID); k++)
						for (int h = 0; h < fields; h++) output << GRID(k)[h] << " ";
				} else if (dim == 3) {
					MMSP::grid<3, MMSP::vector<char> > GRID(fields, lmin, lmax);
					GRID.from_buffer(buffer);
					for (int k = 0; k < MMSP::nodes(GRID); k++)
						for (int h = 0; h < fields; h++) output << GRID(k)[h] << " ";
				}
			}
			if (unsigned_char_type) {
				if (dim == 1) {
					MMSP::grid<1, MMSP::vector<unsigned char> > GRID(fields, lmin, lmax);
					GRID.from_buffer(buffer);
					for (int k = 0; k < MMSP::nodes(GRID); k++)
						for (int h = 0; h < fields; h++) output << GRID(k)[h] << " ";
				} else if (dim == 2) {
					MMSP::grid<2, MMSP::vector<unsigned char> > GRID(fields, lmin, lmax);
					GRID.from_buffer(buffer);
					for (int k = 0; k < MMSP::nodes(GRID); k++)
						for (int h = 0; h < fields; h++) output << GRID(k)[h] << " ";
				} else if (dim == 3) {
					MMSP::grid<3, MMSP::vector<unsigned char> > GRID(fields, lmin, lmax);
					GRID.from_buffer(buffer);
					for (int k = 0; k < MMSP::nodes(GRID); k++)
						for (int h = 0; h < fields; h++) output << GRID(k)[h] << " ";
				}
			}
			if (int_type) {
				if (dim == 1) {
					MMSP::grid<1, MMSP::vector<int> > GRID(fields, lmin, lmax);
					GRID.from_buffer(buffer);
					for (int k = 0; k < MMSP::nodes(GRID); k++)
						for (int h = 0; h < fields; h++) output << GRID(k)[h] << " ";
				} else if (dim == 2) {
					MMSP::grid<2, MMSP::vector<int> > GRID(fields, lmin, lmax);
					GRID.from_buffer(buffer);
					for (int k = 0; k < MMSP::nodes(GRID); k++)
						for (int h = 0; h < fields; h++) output << GRID(k)[h] << " ";
				} else if (dim == 3) {
					MMSP::grid<3, MMSP::vector<int> > GRID(fields, lmin, lmax);
					GRID.from_buffer(buffer);
					for (int k = 0; k < MMSP::nodes(GRID); k++)
						for (int h = 0; h < fields; h++) output << GRID(k)[h] << " ";
				}
			}
			if (unsigned_int_type) {
				if (dim == 1) {
					MMSP::grid<1, MMSP::vector<unsigned int> > GRID(fields, lmin, lmax);
					GRID.from_buffer(buffer);
					for (int k = 0; k < MMSP::nodes(GRID); k++)
						for (int h = 0; h < fields; h++) output << GRID(k)[h] << " ";
				} else if (dim == 2) {
					MMSP::grid<2, MMSP::vector<unsigned int> > GRID(fields, lmin, lmax);
					GRID.from_buffer(buffer);
					for (int k = 0; k < MMSP::nodes(GRID); k++)
						for (int h = 0; h < fields; h++) output << GRID(k)[h] << " ";
				} else if (dim == 3) {
					MMSP::grid<3, MMSP::vector<unsigned int> > GRID(fields, lmin, lmax);
					GRID.from_buffer(buffer);
					for (int k = 0; k < MMSP::nodes(GRID); k++)
						for (int h = 0; h < fields; h++) output << GRID(k)[h] << " ";
				}
			}
			if (long_type) {
				if (dim == 1) {
					MMSP::grid<1, MMSP::vector<long> > GRID(fields, lmin, lmax);
					GRID.from_buffer(buffer);
					for (int k = 0; k < MMSP::nodes(GRID); k++)
						for (int h = 0; h < fields; h++) output << GRID(k)[h] << " ";
				} else if (dim == 2) {
					MMSP::grid<2, MMSP::vector<long> > GRID(fields, lmin, lmax);
					GRID.from_buffer(buffer);
					for (int k = 0; k < MMSP::nodes(GRID); k++)
						for (int h = 0; h < fields; h++) output << GRID(k)[h] << " ";
				} else if (dim == 3) {
					MMSP::grid<3, MMSP::vector<long> > GRID(fields, lmin, lmax);
					GRID.from_buffer(buffer);
					for (int k = 0; k < MMSP::nodes(GRID); k++)
						for (int h = 0; h < fields; h++) output << GRID(k)[h] << " ";
				}
			}
			if (unsigned_long_type) {
				if (dim == 1) {
					MMSP::grid<1, MMSP::vector<unsigned long> > GRID(fields, lmin, lmax);
					GRID.from_buffer(buffer);
					for (int k = 0; k < MMSP::nodes(GRID); k++)
						for (int h = 0; h < fields; h++) output << GRID(k)[h] << " ";
				} else if (dim == 2) {
					MMSP::grid<2, MMSP::vector<unsigned long> > GRID(fields, lmin, lmax);
					GRID.from_buffer(buffer);
					for (int k = 0; k < MMSP::nodes(GRID); k++)
						for (int h = 0; h < fields; h++) output << GRID(k)[h] << " ";
				} else if (dim == 3) {
					MMSP::grid<3, MMSP::vector<unsigned long> > GRID(fields, lmin, lmax);
					GRID.from_buffer(buffer);
					for (int k = 0; k < MMSP::nodes(GRID); k++)
						for (int h = 0; h < fields; h++) output << GRID(k)[h] << " ";
				}
			}
			if (short_type) {
				if (dim == 1) {
					MMSP::grid<1, MMSP::vector<short> > GRID(fields, lmin, lmax);
					GRID.from_buffer(buffer);
					for (int k = 0; k < MMSP::nodes(GRID); k++)
						for (int h = 0; h < fields; h++) output << GRID(k)[h] << " ";
				} else if (dim == 2) {
					MMSP::grid<2, MMSP::vector<short> > GRID(fields, lmin, lmax);
					GRID.from_buffer(buffer);
					for (int k = 0; k < MMSP::nodes(GRID); k++)
						for (int h = 0; h < fields; h++) output << GRID(k)[h] << " ";
				} else if (dim == 3) {
					MMSP::grid<3, MMSP::vector<short> > GRID(fields, lmin, lmax);
					GRID.from_buffer(buffer);
					for (int k = 0; k < MMSP::nodes(GRID); k++)
						for (int h = 0; h < fields; h++) output << GRID(k)[h] << " ";
				}
			}
			if (unsigned_short_type) {
				if (dim == 1) {
					MMSP::grid<1, MMSP::vector<unsigned short> > GRID(fields, lmin, lmax);
					GRID.from_buffer(buffer);
					for (int k = 0; k < MMSP::nodes(GRID); k++)
						for (int h = 0; h < fields; h++) output << GRID(k)[h] << " ";
				} else if (dim == 2) {
					MMSP::grid<2, MMSP::vector<unsigned short> > GRID(fields, lmin, lmax);
					GRID.from_buffer(buffer);
					for (int k = 0; k < MMSP::nodes(GRID); k++)
						for (int h = 0; h < fields; h++) output << GRID(k)[h] << " ";
				} else if (dim == 3) {
					MMSP::grid<3, MMSP::vector<unsigned short> > GRID(fields, lmin, lmax);
					GRID.from_buffer(buffer);
					for (int k = 0; k < MMSP::nodes(GRID); k++)
						for (int h = 0; h < fields; h++) output << GRID(k)[h] << " ";
				}
			}
			if (float_type) {
				if (dim == 1) {
					MMSP::grid<1, MMSP::vector<float> > GRID(fields, lmin, lmax);
					GRID.from_buffer(buffer);
					for (int k = 0; k < MMSP::nodes(GRID); k++)
						for (int h = 0; h < fields; h++) output << GRID(k)[h] << " ";
				} else if (dim == 2) {
					MMSP::grid<2, MMSP::vector<float> > GRID(fields, lmin, lmax);
					GRID.from_buffer(buffer);
					for (int k = 0; k < MMSP::nodes(GRID); k++)
						for (int h = 0; h < fields; h++) output << GRID(k)[h] << " ";
				} else if (dim == 3) {
					MMSP::grid<3, MMSP::vector<float> > GRID(fields, lmin, lmax);
					GRID.from_buffer(buffer);
					for (int k = 0; k < MMSP::nodes(GRID); k++)
						for (int h = 0; h < fields; h++) output << GRID(k)[h] << " ";
				}
			}
			if (double_type) {
				if (dim == 1) {
					MMSP::grid<1, MMSP::vector<double> > GRID(fields, lmin, lmax);
					GRID.from_buffer(buffer);
					for (int k = 0; k < MMSP::nodes(GRID); k++)
						for (int h = 0; h < fields; h++) output << GRID(k)[h] << " ";
				} else if (dim == 2) {
					MMSP::grid<2, MMSP::vector<double> > GRID(fields, lmin, lmax);
					GRID.from_buffer(buffer);
					for (int k = 0; k < MMSP::nodes(GRID); k++)
						for (int h = 0; h < fields; h++) output << GRID(k)[h] << " ";
				} else if (dim == 3) {
					MMSP::grid<3, MMSP::vector<double> > GRID(fields, lmin, lmax);
					GRID.from_buffer(buffer);
					for (int k = 0; k < MMSP::nodes(GRID); k++)
						for (int h = 0; h < fields; h++) output << GRID(k)[h] << " ";
				}
			}
			if (long_double_type) {
				if (dim == 1) {
					MMSP::grid<1, MMSP::vector<long double> > GRID(fields, lmin, lmax);
					GRID.from_buffer(buffer);
					for (int k = 0; k < MMSP::nodes(GRID); k++)
						for (int h = 0; h < fields; h++) output << GRID(k)[h] << " ";
				} else if (dim == 2) {
					MMSP::grid<2, MMSP::vector<long double> > GRID(fields, lmin, lmax);
					GRID.from_buffer(buffer);
					for (int k = 0; k < MMSP::nodes(GRID); k++)
						for (int h = 0; h < fields; h++) output << GRID(k)[h] << " ";
				} else if (dim == 3) {
					MMSP::grid<3, MMSP::vector<long double> > GRID(fields, lmin, lmax);
					GRID.from_buffer(buffer);
					for (int k = 0; k < MMSP::nodes(GRID); k++)
						for (int h = 0; h < fields; h++) output << GRID(k)[h] << " ";
				}
			}
		}

		else if (sparse_type) {
			if (bool_type) {
				if (dim == 1) {
					MMSP::grid<1, MMSP::sparse<bool> > GRID(fields, lmin, lmax);
					GRID.from_buffer(buffer);
					for (int k = 0; k < MMSP::nodes(GRID); k++) {
						bool sum = 0;
						int nonzero = MMSP::length(GRID(k));
						for (int h = 0; h < nonzero; h++)
							sum += static_cast<bool>(GRID(k).value(h) * GRID(k).value(h));
						output << sum << " ";
					}
				} else if (dim == 2) {
					MMSP::grid<2, MMSP::sparse<bool> > GRID(fields, lmin, lmax);
					GRID.from_buffer(buffer);
					for (int k = 0; k < MMSP::nodes(GRID); k++) {
						bool sum = 0;
						int nonzero = MMSP::length(GRID(k));
						for (int h = 0; h < nonzero; h++)
							sum += static_cast<bool>(GRID(k).value(h) * GRID(k).value(h));
						output << sum << " ";
					}
				} else if (dim == 3) {
					MMSP::grid<3, MMSP::sparse<bool> > GRID(fields, lmin, lmax);
					GRID.from_buffer(buffer);
					for (int k = 0; k < MMSP::nodes(GRID); k++) {
						bool sum = 0;
						int nonzero = MMSP::length(GRID(k));
						for (int h = 0; h < nonzero; h++)
							sum += static_cast<bool>(GRID(k).value(h) * GRID(k).value(h));
						output << sum << " ";
					}
				}
			}
			if (char_type) {
				if (dim == 1) {
					MMSP::grid<1, MMSP::sparse<char> > GRID(fields, lmin, lmax);
					GRID.from_buffer(buffer);
					for (int k = 0; k < MMSP::nodes(GRID); k++) {
						char sum = 0;
						int nonzero = MMSP::length(GRID(k));
						for (int h = 0; h < nonzero; h++)
							sum += static_cast<char>(GRID(k).value(h) * GRID(k).value(h));
						output << sum << " ";
					}
				} else if (dim == 2) {
					MMSP::grid<2, MMSP::sparse<char> > GRID(fields, lmin, lmax);
					GRID.from_buffer(buffer);
					for (int k = 0; k < MMSP::nodes(GRID); k++) {
						char sum = 0;
						int nonzero = MMSP::length(GRID(k));
						for (int h = 0; h < nonzero; h++)
							sum += static_cast<char>(GRID(k).value(h) * GRID(k).value(h));
						output << sum << " ";
					}
				} else if (dim == 3) {
					MMSP::grid<3, MMSP::sparse<char> > GRID(fields, lmin, lmax);
					GRID.from_buffer(buffer);
					for (int k = 0; k < MMSP::nodes(GRID); k++) {
						char sum = 0;
						int nonzero = MMSP::length(GRID(k));
						for (int h = 0; h < nonzero; h++)
							sum += static_cast<char>(GRID(k).value(h) * GRID(k).value(h));
						output << sum << " ";
					}
				}
			}
			if (unsigned_char_type) {
				if (dim == 1) {
					MMSP::grid<1, MMSP::sparse<unsigned char> > GRID(fields, lmin, lmax);
					GRID.from_buffer(buffer);
					for (int k = 0; k < MMSP::nodes(GRID); k++) {
						unsigned char sum = 0;
						int nonzero = MMSP::length(GRID(k));
						for (int h = 0; h < nonzero; h++)
							sum += static_cast<unsigned char>(GRID(k).value(h) * GRID(k).value(h));
						output << sum << " ";
					}
				} else if (dim == 2) {
					MMSP::grid<2, MMSP::sparse<unsigned char> > GRID(fields, lmin, lmax);
					GRID.from_buffer(buffer);
					for (int k = 0; k < MMSP::nodes(GRID); k++) {
						unsigned char sum = 0;
						int nonzero = MMSP::length(GRID(k));
						for (int h = 0; h < nonzero; h++)
							sum += static_cast<unsigned char>(GRID(k).value(h) * GRID(k).value(h));
						output << sum << " ";
					}
				} else if (dim == 3) {
					MMSP::grid<3, MMSP::sparse<unsigned char> > GRID(fields, lmin, lmax);
					GRID.from_buffer(buffer);
					for (int k = 0; k < MMSP::nodes(GRID); k++) {
						unsigned char sum = 0;
						int nonzero = MMSP::length(GRID(k));
						for (int h = 0; h < nonzero; h++)
							sum += static_cast<unsigned char>(GRID(k).value(h) * GRID(k).value(h));
						output << sum << " ";
					}
				}
			}
			if (int_type) {
				if (dim == 1) {
					MMSP::grid<1, MMSP::sparse<int> > GRID(fields, lmin, lmax);
					GRID.from_buffer(buffer);
					for (int k = 0; k < MMSP::nodes(GRID); k++) {
						int sum = 0;
						int nonzero = MMSP::length(GRID(k));
						for (int h = 0; h < nonzero; h++)
							sum += static_cast<int>(GRID(k).value(h) * GRID(k).value(h));
						output << sum << " ";
					}
				} else if (dim == 2) {
					MMSP::grid<2, MMSP::sparse<int> > GRID(fields, lmin, lmax);
					GRID.from_buffer(buffer);
					for (int k = 0; k < MMSP::nodes(GRID); k++) {
						int sum = 0;
						int nonzero = MMSP::length(GRID(k));
						for (int h = 0; h < nonzero; h++)
							sum += static_cast<int>(GRID(k).value(h) * GRID(k).value(h));
						output << sum << " ";
					}
				} else if (dim == 3) {
					MMSP::grid<3, MMSP::sparse<int> > GRID(fields, lmin, lmax);
					GRID.from_buffer(buffer);
					for (int k = 0; k < MMSP::nodes(GRID); k++) {
						int sum = 0;
						int nonzero = MMSP::length(GRID(k));
						for (int h = 0; h < nonzero; h++)
							sum += static_cast<int>(GRID(k).value(h) * GRID(k).value(h));
						output << sum << " ";
					}
				}
			}
			if (unsigned_int_type) {
				if (dim == 1) {
					MMSP::grid<1, MMSP::sparse<unsigned int> > GRID(fields, lmin, lmax);
					GRID.from_buffer(buffer);
					for (int k = 0; k < MMSP::nodes(GRID); k++) {
						unsigned int sum = 0;
						int nonzero = MMSP::length(GRID(k));
						for (int h = 0; h < nonzero; h++)
							sum += static_cast<unsigned int>(GRID(k).value(h) * GRID(k).value(h));
						output << sum << " ";
					}
				} else if (dim == 2) {
					MMSP::grid<2, MMSP::sparse<unsigned int> > GRID(fields, lmin, lmax);
					GRID.from_buffer(buffer);
					for (int k = 0; k < MMSP::nodes(GRID); k++) {
						unsigned int sum = 0;
						int nonzero = MMSP::length(GRID(k));
						for (int h = 0; h < nonzero; h++)
							sum += static_cast<unsigned int>(GRID(k).value(h) * GRID(k).value(h));
						output << sum << " ";
					}
				} else if (dim == 3) {
					MMSP::grid<3, MMSP::sparse<unsigned int> > GRID(fields, lmin, lmax);
					GRID.from_buffer(buffer);
					for (int k = 0; k < MMSP::nodes(GRID); k++) {
						unsigned int sum = 0;
						int nonzero = MMSP::length(GRID(k));
						for (int h = 0; h < nonzero; h++)
							sum += static_cast<unsigned int>(GRID(k).value(h) * GRID(k).value(h));
						output << sum << " ";
					}
				}
			}
			if (long_type) {
				if (dim == 1) {
					MMSP::grid<1, MMSP::sparse<long> > GRID(fields, lmin, lmax);
					GRID.from_buffer(buffer);
					for (int k = 0; k < MMSP::nodes(GRID); k++) {
						long sum = 0;
						int nonzero = MMSP::length(GRID(k));
						for (int h = 0; h < nonzero; h++)
							sum += static_cast<long>(GRID(k).value(h) * GRID(k).value(h));
						output << sum << " ";
					}
				} else if (dim == 2) {
					MMSP::grid<2, MMSP::sparse<long> > GRID(fields, lmin, lmax);
					GRID.from_buffer(buffer);
					for (int k = 0; k < MMSP::nodes(GRID); k++) {
						long sum = 0;
						int nonzero = MMSP::length(GRID(k));
						for (int h = 0; h < nonzero; h++)
							sum += static_cast<long>(GRID(k).value(h) * GRID(k).value(h));
						output << sum << " ";
					}
				} else if (dim == 3) {
					MMSP::grid<3, MMSP::sparse<long> > GRID(fields, lmin, lmax);
					GRID.from_buffer(buffer);
					for (int k = 0; k < MMSP::nodes(GRID); k++) {
						long sum = 0;
						int nonzero = MMSP::length(GRID(k));
						for (int h = 0; h < nonzero; h++)
							sum += static_cast<long>(GRID(k).value(h) * GRID(k).value(h));
						output << sum << " ";
					}
				}
			}
			if (unsigned_long_type) {
				if (dim == 1) {
					MMSP::grid<1, MMSP::sparse<unsigned long> > GRID(fields, lmin, lmax);
					GRID.from_buffer(buffer);
					for (int k = 0; k < MMSP::nodes(GRID); k++) {
						unsigned long sum = 0;
						int nonzero = MMSP::length(GRID(k));
						for (int h = 0; h < nonzero; h++)
							sum += static_cast<unsigned long>(GRID(k).value(h) * GRID(k).value(h));
						output << sum << " ";
					}
				} else if (dim == 2) {
					MMSP::grid<2, MMSP::sparse<unsigned long> > GRID(fields, lmin, lmax);
					GRID.from_buffer(buffer);
					for (int k = 0; k < MMSP::nodes(GRID); k++) {
						unsigned long sum = 0;
						int nonzero = MMSP::length(GRID(k));
						for (int h = 0; h < nonzero; h++)
							sum += static_cast<unsigned long>(GRID(k).value(h) * GRID(k).value(h));
						output << sum << " ";
					}
				} else if (dim == 3) {
					MMSP::grid<3, MMSP::sparse<unsigned long> > GRID(fields, lmin, lmax);
					GRID.from_buffer(buffer);
					for (int k = 0; k < MMSP::nodes(GRID); k++) {
						unsigned long sum = 0;
						int nonzero = MMSP::length(GRID(k));
						for (int h = 0; h < nonzero; h++)
							sum += static_cast<unsigned long>(GRID(k).value(h) * GRID(k).value(h));
						output << sum << " ";
					}
				}
			}
			if (short_type) {
				if (dim == 1) {
					MMSP::grid<1, MMSP::sparse<short> > GRID(fields, lmin, lmax);
					GRID.from_buffer(buffer);
					for (int k = 0; k < MMSP::nodes(GRID); k++) {
						short sum = 0;
						int nonzero = MMSP::length(GRID(k));
						for (int h = 0; h < nonzero; h++)
							sum += static_cast<short>(GRID(k).value(h) * GRID(k).value(h));
						output << sum << " ";
					}
				} else if (dim == 2) {
					MMSP::grid<2, MMSP::sparse<short> > GRID(fields, lmin, lmax);
					GRID.from_buffer(buffer);
					for (int k = 0; k < MMSP::nodes(GRID); k++) {
						short sum = 0;
						int nonzero = MMSP::length(GRID(k));
						for (int h = 0; h < nonzero; h++)
							sum += static_cast<short>(GRID(k).value(h) * GRID(k).value(h));
						output << sum << " ";
					}
				} else if (dim == 3) {
					MMSP::grid<3, MMSP::sparse<short> > GRID(fields, lmin, lmax);
					GRID.from_buffer(buffer);
					for (int k = 0; k < MMSP::nodes(GRID); k++) {
						short sum = 0;
						int nonzero = MMSP::length(GRID(k));
						for (int h = 0; h < nonzero; h++)
							sum += static_cast<short>(GRID(k).value(h) * GRID(k).value(h));
						output << sum << " ";
					}
				}
			}
			if (unsigned_short_type) {
				if (dim == 1) {
					MMSP::grid<1, MMSP::sparse<unsigned short> > GRID(fields, lmin, lmax);
					GRID.from_buffer(buffer);
					for (int k = 0; k < MMSP::nodes(GRID); k++) {
						unsigned short sum = 0;
						int nonzero = MMSP::length(GRID(k));
						for (int h = 0; h < nonzero; h++)
							sum += static_cast<unsigned short>(GRID(k).value(h) * GRID(k).value(h));
						output << sum << " ";
					}
				} else if (dim == 2) {
					MMSP::grid<2, MMSP::sparse<unsigned short> > GRID(fields, lmin, lmax);
					GRID.from_buffer(buffer);
					for (int k = 0; k < MMSP::nodes(GRID); k++) {
						unsigned short sum = 0;
						int nonzero = MMSP::length(GRID(k));
						for (int h = 0; h < nonzero; h++)
							sum += static_cast<unsigned short>(GRID(k).value(h) * GRID(k).value(h));
						output << sum << " ";
					}
				} else if (dim == 3) {
					MMSP::grid<3, MMSP::sparse<unsigned short> > GRID(fields, lmin, lmax);
					GRID.from_buffer(buffer);
					for (int k = 0; k < MMSP::nodes(GRID); k++) {
						unsigned short sum = 0;
						int nonzero = MMSP::length(GRID(k));
						for (int h = 0; h < nonzero; h++)
							sum += static_cast<unsigned short>(GRID(k).value(h) * GRID(k).value(h));
						output << sum << " ";
					}
				}
			}
			if (float_type) {
				if (dim == 1) {
					MMSP::grid<1, MMSP::sparse<float> > GRID(fields, lmin, lmax);
					GRID.from_buffer(buffer);
					for (int k = 0; k < MMSP::nodes(GRID); k++) {
						float sum = 0;
						int nonzero = MMSP::length(GRID(k));
						for (int h = 0; h < nonzero; h++)
							sum += static_cast<float>(GRID(k).value(h) * GRID(k).value(h));
						output << sum << " ";
					}
				} else if (dim == 2) {
					MMSP::grid<2, MMSP::sparse<float> > GRID(fields, lmin, lmax);
					GRID.from_buffer(buffer);
					for (int k = 0; k < MMSP::nodes(GRID); k++) {
						float sum = 0;
						int nonzero = MMSP::length(GRID(k));
						for (int h = 0; h < nonzero; h++)
							sum += static_cast<float>(GRID(k).value(h) * GRID(k).value(h));
						output << sum << " ";
					}
				} else if (dim == 3) {
					MMSP::grid<3, MMSP::sparse<float> > GRID(fields, lmin, lmax);
					GRID.from_buffer(buffer);
					for (int k = 0; k < MMSP::nodes(GRID); k++) {
						float sum = 0;
						int nonzero = MMSP::length(GRID(k));
						for (int h = 0; h < nonzero; h++)
							sum += static_cast<float>(GRID(k).value(h) * GRID(k).value(h));
						output << sum << " ";
					}
				}
			}
			if (double_type) {
				if (dim == 1) {
					MMSP::grid<1, MMSP::sparse<double> > GRID(fields, lmin, lmax);
					GRID.from_buffer(buffer);
					for (int k = 0; k < MMSP::nodes(GRID); k++) {
						double sum = 0;
						int nonzero = MMSP::length(GRID(k));
						for (int h = 0; h < nonzero; h++)
							sum += static_cast<double>(GRID(k).value(h) * GRID(k).value(h));
						output << sum << " ";
					}
				} else if (dim == 2) {
					MMSP::grid<2, MMSP::sparse<double> > GRID(fields, lmin, lmax);
					GRID.from_buffer(buffer);
					for (int k = 0; k < MMSP::nodes(GRID); k++) {
						double sum = 0;
						int nonzero = MMSP::length(GRID(k));
						for (int h = 0; h < nonzero; h++)
							sum += static_cast<double>(GRID(k).value(h) * GRID(k).value(h));
						output << sum << " ";
					}
				} else if (dim == 3) {
					MMSP::grid<3, MMSP::sparse<double> > GRID(fields, lmin, lmax);
					GRID.from_buffer(buffer);
					for (int k = 0; k < MMSP::nodes(GRID); k++) {
						double sum = 0;
						int nonzero = MMSP::length(GRID(k));
						for (int h = 0; h < nonzero; h++)
							sum += static_cast<double>(GRID(k).value(h) * GRID(k).value(h));
						output << sum << " ";
					}
				}
			}
			if (long_double_type) {
				if (dim == 1) {
					MMSP::grid<1, MMSP::sparse<long double> > GRID(fields, lmin, lmax);
					GRID.from_buffer(buffer);
					for (int k = 0; k < MMSP::nodes(GRID); k++) {
						long double sum = 0;
						int nonzero = MMSP::length(GRID(k));
						for (int h = 0; h < nonzero; h++)
							sum += static_cast<long double>(GRID(k).value(h) * GRID(k).value(h));
						output << sum << " ";
					}
				} else if (dim == 2) {
					MMSP::grid<2, MMSP::sparse<long double> > GRID(fields, lmin, lmax);
					GRID.from_buffer(buffer);
					for (int k = 0; k < MMSP::nodes(GRID); k++) {
						long double sum = 0;
						int nonzero = MMSP::length(GRID(k));
						for (int h = 0; h < nonzero; h++)
							sum += static_cast<long double>(GRID(k).value(h) * GRID(k).value(h));
						output << sum << " ";
					}
				} else if (dim == 3) {
					MMSP::grid<3, MMSP::sparse<long double> > GRID(fields, lmin, lmax);
					GRID.from_buffer(buffer);
					for (int k = 0; k < MMSP::nodes(GRID); k++) {
						long double sum = 0;
						int nonzero = MMSP::length(GRID(k));
						for (int h = 0; h < nonzero; h++)
							sum += static_cast<long double>(GRID(k).value(h) * GRID(k).value(h));
						output << sum << " ";
					}
				}
			}
		}

		// clean up
		delete [] buffer;
  }

  // write closing markup
		output << "\n";
		output << "        </DataArray>\n";
		output << "      </CellData>\n";
		output << "    </Piece>\n";
  unsigned long grain_num = (*grain_ids).size();
std::cout<<"grain_num "<<(*grain_ids).size()<<std::endl;
  double line_length = 0.25*3.1415*2.0*(r_fusion+distance) + Tv*physical_time;
std::cout<<"line_length "<<line_length<<std::endl;
  double grain_size = line_length/grain_num;
  std::cout<<"grain size at "<<distance<<" from fusion line is "<<grain_size<<std::endl;
  (*grain_ids).clear();

}
