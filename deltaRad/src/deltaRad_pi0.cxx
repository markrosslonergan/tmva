/*#include "object_helper.h"
#include "get_pot_info.h"
#include "train.h"
#include "app.h"
#include "merge.h"
#include "significance.h"
#include "get_mva_response_hists.h"
#include "plot_mva_response_hists.h"
#include "gen_tlimits.h"
#include "plotstack.h"
*/
#include "load_mva_param.h"
#include "tinyxml.h"

#include <getopt.h>


#include "bdt_file.h"
#include "bdt_var.h"
#include "bdt_info.h"
#include "bdt_train.h"
#include "bdt_app.h"
#include "bdt_response.h"
#include "bdt_recomc.h"
#include "bdt_sig.h"
#include "bdt_spec.h"

#include "bdt_precalc.h"



int main (int argc, char *argv[]){

	// Just some simple argument things
	//===========================================================================================
	std::string dir = "/home/mark/work/uBooNE/photon/tmva/";

	//std::string dir = "/home/amogan/singlePhotonCode/hellstroms_hive/";
	std::string mode_option = "train"; 
	std::string xml = "default.xml";
	std::string istrack ="track";

	const struct option longopts[] = 
	{
		{"dir", 		required_argument, 	0, 'd'},
		{"option",		required_argument,	0, 'o'},
		{"xml"	,		required_argument,	0, 'x'},
		{"track",		required_argument,	0, 't'},
		{"help",		required_argument,	0, 'h'},
		{0,			no_argument, 		0,  0},
	};

	//some optioni/argument stuff
	int iarg = 0; opterr=1; int index;
	while(iarg != -1)
	{
		iarg = getopt_long(argc,argv, "x:o:d:t:h?", longopts, &index);

		switch(iarg)
		{
			case 'x':
				xml = optarg;
				break;
			case 'o':
				mode_option = optarg;
				break;
			case 'd':
				dir = optarg;
				break;
			case 't':
				istrack = optarg;
				break;
			case '?':
			case 'h':
				std::cout<<"Allowed arguments:"<<std::endl;
				std::cout<<"\t-d\t--dir\t\tDirectory for file inputs"<<std::endl;
				std::cout<<"\t-o\t--option\t\tOptional mode to run, train, app..etc.."<<std::endl;
				std::cout<<"\t-t\t--track\t\tQuickly run between track and notrack"<<std::endl;
				std::cout<<"\t-x\t--xml\t\tInput .xml file for configuring what MVA/BDT & param"<<std::endl;
				std::cout<<"\t-h\t--help\t\tThis help menu"<<std::endl;
				return 0;
		}

	}


	TFile * ftest = new TFile(("test+"+istrack+".root").c_str(),"recreate");
	//===========================================================================================
	//===========================================================================================
	//===========================================================================================
	//===========================================================================================

	
	MVALoader xml_methods(xml);
	std::vector<method_struct> TMVAmethods  = xml_methods.GetMethods(); 


	//Define the precuts, depending on if you want to use track or notrack	
	std::string new_precuts;
    std::string num_track_cut;
	if(istrack == "track"){
	    //num_track_cut = " > 0 ";
	    num_track_cut = " ==1 ";
		//new_precuts =  "reco_nu_vtx_dist_to_closest_tpc_wall > 10 && shortest_asso_shower_to_vert_dist > 2 && longest_asso_track_displacement < 150 && totalpe_ibg_sum >50";
        new_precuts = "1";
		//new_precuts =  "reco_nu_vtx_dist_to_closest_tpc_wall > 10 && shortest_asso_shower_to_vert_dist > 2 && reco_track_displacement[longest_asso_track_index] < 150 && totalpe_ibg_sum >50";

	}else if(istrack == "notrack"){
		//new_precuts = "reco_nu_vtx_dist_to_closest_tpc_wall > 10 && totalpe_ibg_sum > 50";
		new_precuts = "1";
		num_track_cut = "==0";
	}


	//Set up 2 bdt_info structs for passing information on what BDT we are running.
	bdt_info bnb_bdt_info("pi0bnb_"+istrack, "BNB focused BDT");
	bdt_info cosmic_bdt_info("pi0cosmic_"+istrack, "Cosmic focused BDT");
	

	// Our signal definition alongside any base cuts we want to make
	std::string base_cuts = "reco_asso_showers == 2 && reco_asso_tracks " + num_track_cut;
	std::string cosmic_base_cuts = "reco_asso_showers == 2 && reco_asso_tracks " + num_track_cut;

	std::string signal_definition = "ccnc==1 &&true_shower_parent_pdg[second_most_energetic_shower_index]==111";
	std::string background_definition = "!(" + signal_definition + ")";

	
	//This is a particular cut flow that a file will undergo. I.e base cuts, precuts, postcuts, and then the name of the Cosmic BDT and bnb bdt
	bdt_flow signal_flow(base_cuts +"&&"+signal_definition,	new_precuts,"1", 	cosmic_bdt_info.identifier,bnb_bdt_info.identifier);
	bdt_flow cosmic_flow(cosmic_base_cuts, 			new_precuts,"1", 	cosmic_bdt_info.identifier,bnb_bdt_info.identifier);
	bdt_flow bkg_flow(base_cuts +"&&"+background_definition,new_precuts,"1",		cosmic_bdt_info.identifier ,bnb_bdt_info.identifier);
	bdt_flow data_flow(base_cuts ,				new_precuts,"1",		cosmic_bdt_info.identifier, bnb_bdt_info.identifier);


	// BDT files, in the form (location, rootfilt, name, hisotgram_options, tfile_folder, tag, color, bdt_flow )		

	bdt_file *signal_pure    = new bdt_file(dir+"samples/mcc86/", "VR_bnb_2000.root",		     "NCpi0Pure", "hist","",  kBlue-4, signal_flow);
	bdt_file *signal_cosmics = new bdt_file(dir+"samples/bnbcosmic/", "merged_bnbcosmic_mcc88_v1.0.root", "NCpi0Cosmics", "hist","",  kBlue-4, signal_flow);
	bdt_file *bnb_pure    = new bdt_file(dir+"samples/bnbcosmic/", "merged_bnbcosmic_mcc88_v1.0.root",	"BNBPure",	   "hist","",  kRed-6, bkg_flow);
	//bdt_file *bnb_cosmics = new bdt_file(dir+"samples/mcc86/", "merged.bnbcosmic_v3.0_mcc86_withcalo.root", "BNBCosmics", "hist","",  kBlue-4, bkg_flow);
	bdt_file *bnb_cosmics = new bdt_file(dir+"samples/bnbcosmic/", "merged_bnbcosmic_mcc88_v1.0.root", "BNBCosmics", "hist","",  kRed-6, bkg_flow);

	//bdt_file *intime = new bdt_file(dir+"samples/mcc86/", "merged.intime_v1.0.root" ,"IntimeCosmics","hist","", kGreen-3, cosmic_flow);
	bdt_file *intime = new bdt_file(dir+"samples/mcc86/", "VR_bnb_cosmic_2000.root" ,"IntimeCosmics","hist","", kGreen-3, cosmic_flow);


	std::vector<bdt_file*> bdt_files = {signal_pure, signal_cosmics, bnb_pure, bnb_cosmics};

    /*
	//Add Any other info in the form of FRIENDS! e.g track dEdx 
	signal_pure->addFriend("track_dEdx_tree",dir+"track_dEdx/trackdEdx_nsignal.root");			
	signal_cosmics->addFriend("track_dEdx_tree",dir+"track_dEdx/trackdEdx_nsignal_cosmics.root");			
	bnb_pure->addFriend("track_dEdx_tree",dir+"track_dEdx/trackdEdx_bnb.root");			
	bnb_cosmics->addFriend("track_dEdx_tree",dir+"track_dEdx/trackdEdx_bnb_cosmics.root");			
	intime->addFriend("track_dEdx_tree",dir+"track_dEdx/trackdEdx_intime.root");			
//	data5e19->addFriend("track_dEdx_tree",dir+"track_dEdx/trackdEdx_data5e19.root");			
//	bnbext->addFriend("track_dEdx_tree",dir+"track_dEdx/trackdEdx_bnbext.root");			
	//data5e19->addFriend("track_dEdx_tree",dir+"track_dEdx/trackdEdx_data5e19.root");			
	//bnbext->addFriend("track_dEdx_tree",dir+"track_dEdx/trackdEdx_bnbext.root");			
    */


	//Variables!
	std::string angle_track_shower1 ="(reco_track_dirx[longest_asso_track_index]*reco_shower_dirx[most_energetic_shower_index]+reco_track_diry[longest_asso_track_index]*reco_shower_diry[most_energetic_shower_index]+reco_track_dirz[longest_asso_track_index]*reco_shower_dirz[most_energetic_shower_index])";
	std::string angle_track_shower2 ="(reco_track_dirx[longest_asso_track_index]*reco_shower_dirx[second_most_energetic_shower_index]+reco_track_diry[longest_asso_track_index]*reco_shower_diry[second_most_energetic_shower_index]+reco_track_dirz[longest_asso_track_index]*reco_shower_dirz[second_most_energetic_shower_index])";
	//std::string angle_shower1_shower2 ="reco_shower_dirx[most_energetic_shower_index]*reco_shower_dirx[second_most_energetic_shower_index]+reco_shower_diry[most_energetic_shower_index]*reco_shower_diry[second_most_energetic_shower_index]+reco_shower_dirz[most_energetic_shower_index]*reco_shower_dirz[second_most_energetic_shower_index]";
	std::string angle_shower1_shower2 ="reco_shower_dirx[0]*reco_shower_dirx[1]+reco_shower_diry[0]*reco_shower_diry[1]+reco_shower_dirz[0]*reco_shower_dirz[1]";
	//std::string angle_shower1_shower2 ="";
    std::vector<bdt_variable> vars;
   

    // Calculate invariant mass; note that p's here are 3-momenta
    std::string invMass;
    if (true) {
        std::string E1 = "reco_shower_helper_energy[0]"; 
        std::string px1 = "reco_shower_helper_energy[0]*reco_shower_dirx[0]";
        std::string py1 = "reco_shower_helper_energy[0]*reco_shower_diry[0]";
        std::string pz1 = "reco_shower_helper_energy[0]*reco_shower_dirz[0]";
        std::string E2 = "reco_shower_helper_energy[1]"; 
        std::string px2 = "reco_shower_helper_energy[1]*reco_shower_dirx[1]";
        std::string py2 = "reco_shower_helper_energy[1]*reco_shower_diry[1]";
        std::string pz2 = "reco_shower_helper_energy[1]*reco_shower_dirz[1]";
        std::string p1Squared = px1 + "*" + px1 + "+" + py1 + "*" + py1 + "+" + pz1 + "*" + pz1;
        std::string p2Squared = px2 + "*" + px2 + "+" + py2 + "*" + py2 + "+" + pz2 + "*" + pz2;
        std::string p1Dotp2 = px1 + "*" + px2+ "+" + py1+ "*" +py2+ "+" + pz1+ "*" + pz2;
        // Invariant mass = E1^2 - p1^2 + E2^2 - p2^2 + 2(E1E2 - p1*p2) ("*" denotes three-momentum dot product)
        invMass = E1 + "*" + E1 + "-" + p1Squared + "+" + E2 + "*" + E2 + "-" + p2Squared + " + 2*(" + E1 + "*" + E2 + "-" + p1Dotp2 + ")";

        invMass = "sqrt(2.0*"+E1+"*"+E2+"*(1.0-"+angle_shower1_shower2+"))";
    }
    vars.push_back(bdt_variable(invMass, "(20, 0., 0.5)", "Shower Invariant Mass [MeV/c]", false, "d"));




	//vars.push_back(bdt_variable("reco_shower_dedx_plane2","(48,0,15)", "Shower dE/dx Collection Plane [MeV/cm]",false,"d"));
	//vars.push_back(bdt_variable("reco_shower_dedx_plane2[most_energetic_shower_index]","(48,0,15)", "Shower dE/dx Collection Plane [MeV/cm]",false,"d"));
	//vars.push_back(bdt_variable("reco_shower_dedx_plane2[second_most_energetic_shower_index]","(48,0,15)", "Shower dE/dx Collection Plane [MeV/cm]",false,"d"));
	vars.push_back(bdt_variable("reco_shower_dedx_plane2[most_energetic_shower_index]","(48,0,15)", "Shower 1 dE/dx Collection Plane [MeV/cm]",false,"d"));
	vars.push_back(bdt_variable("reco_shower_dedx_plane2[second_most_energetic_shower_index]","(48,0,15)", "Shower 2 dE/dx Collection Plane [MeV/cm]",false,"d"));
	vars.push_back(bdt_variable("summed_associated_helper_shower_energy","(25,0,0.5)","Summed Shower Energy [GeV]", false,"d"));
	//vars.push_back(bdt_variable("most_energetic_shower_reco_length","(25,0,125)","Shower Length [cm]",false,"d"));
	vars.push_back(bdt_variable("reco_shower_length[most_energetic_shower_index]","(25,0,125)","Most Shower Length [cm]",false,"d"));
	vars.push_back(bdt_variable("reco_shower_length[second_most_energetic_shower_index]","(25,0,125)","Least Shower Length [cm]",false,"d"));
	//vars.push_back(bdt_variable("reco_shower_length","(25,0,125)","Shower Length [cm]",false,"d"));
	//vars.push_back(bdt_variable("reco_shower_length[most_energetic_shower_index]","(25,0,125)","Shower Length [cm]",false,"d"));
	//vars.push_back(bdt_variable("reco_shower_length[second_most_energetic_shower_index]","(25,0,125)","Shower Length [cm]",false,"d"));
    vars.push_back(bdt_variable(angle_track_shower1, "(25, -1, 1)", "Angle b/n Track Shower 1", false, "d"));
    vars.push_back(bdt_variable(angle_track_shower2, "(25, -1, 1)", "Angle b/n Track Shower 2", false, "d"));
    vars.push_back(bdt_variable(angle_shower1_shower2, "(25, -1, 1)", "Opening Angle between Showers", false, "d"));

	vars.push_back(bdt_variable("totalpe_ibg_sum","(25,0,2000)","Total in Beam-Gate PE",false,"d"));
	vars.push_back(bdt_variable("closest_asso_shower_dist_to_flashzcenter","(25,0,1000)","Distance from Shower to Flashcenter [cm]",false,"d"));

	vars.push_back(bdt_variable("reco_nu_vtx_dist_to_closest_tpc_wall","(25,0,125)","Reconstructed Vertex to TPC Wall Distance [cm]",false,"d"));
	//vars.push_back(bdt_variable("most_energetic_shower_bp_dist_to_tpc","(25,0,1000)","Back Projected Distance from Shower to TPC wall [cm]",false,"d"));
	vars.push_back(bdt_variable("reco_shower_bp_dist_to_tpc[most_energetic_shower_index]","(25,0,1000)","Back Projected Distance from Shower 1 to TPC wall [cm]",false,"d"));
	vars.push_back(bdt_variable("reco_shower_bp_dist_to_tpc[second_most_energetic_shower_index]","(25,0,1000)","Back Projected Distance from Shower 2 to TPC wall [cm]",false,"d"));
	vars.push_back(bdt_variable("reco_nuvertx","(25,0,300)"," Reconstructed Vertex x-location [cm]",false,"d"));
	vars.push_back(bdt_variable("reco_nuverty","(25,-300,+300)","Reconstructed Vertex y-location [cm]",false,"d"));
	vars.push_back(bdt_variable("reco_nuvertz","(25,0,1000)","Reconstructed Vertex z-location [cm]",false,"d"));

	//vars.push_back(bdt_variable("cos(atan(most_energetic_shower_reco_diry/most_energetic_shower_reco_dirz))","(25,0,1)","Reconstructed Shower |Cosine Theta|", true,"d"));
	//vars.push_back(bdt_variable("cos(atan(most_energetic_shower_reco_diry/most_energetic_shower_reco_dirx))","(25,0,1)","Reconstructed Shower |Cosine Phi|", true,"d"));
	vars.push_back(bdt_variable("cos(atan(reco_shower_diry[most_energetic_shower_index]/reco_shower_dirz[most_energetic_shower_index]))","(25,0,1)","Reconstructed Shower 1 |Cosine Theta|", true,"d"));
	vars.push_back(bdt_variable("cos(atan(reco_shower_diry[most_energetic_shower_index]/reco_shower_dirx[most_energetic_shower_index]))","(25,0,1)","Reconstructed Shower 2 |Cosine Phi|", true,"d"));
	vars.push_back(bdt_variable("cos(atan(reco_shower_diry[second_most_energetic_shower_index]/reco_shower_dirz[second_most_energetic_shower_index]))","(25,0,1)","Reconstructed Shower 1 |Cosine Theta|", true,"d"));
	vars.push_back(bdt_variable("cos(atan(reco_shower_diry[second_most_energetic_shower_index]/reco_shower_dirx[second_most_energetic_shower_index]))","(25,0,1)","Reconstructed Shower 2 |Cosine Phi|", true,"d"));
	if(istrack=="track"){

//		vars.push_back(bdt_variable("track_dEdx_tree.longest_asso_track_range","(25,0,500)","Reconstructed Track Range? [cm]", true,"d"));
		//vars.push_back(bdt_variable("longest_asso_track_displacement","(25,0,500)","Reconstructed Track Length [cm]", true,"d"));
        
		vars.push_back(bdt_variable("reco_track_displacement[longest_asso_track_index]","(25,0,500)","Reconstructed Track Length [cm]", true,"d"));
		//vars.push_back(bdt_variable("reco_track_displacement[0]","(25,0,500)","Reconstructed Track Length [cm]", true,"d"));
//		vars.push_back(bdt_variable("reco_track_displacement[longest_asso_track_index+1]","(25,0,500)","Reconstructed Track Length [cm]", true,"d"));
		//vars.push_back(bdt_variable("reco_track_displacement","(25,0,500)","Reconstructed Track Length [cm]", true,"d"));
        // Above line doesn't work! But deleing the index off of shower variables works fine!?!??!?!
        
		//vars.push_back(bdt_variable("track_dEdx_tree.longest_asso_track_bragg_start_parD", "(25,-2,2)","Bragg Parameter D", true,"d"));
		vars.push_back(bdt_variable("shortest_asso_shower_to_vert_dist","(50,0,20)","Closest Photon Conversion Length from Reconstructed Vertex [cm]" ,false,"d"));
	//	vars.push_back(bdt_variable("track_dEdx_tree.longest_asso_track_bragg_start_parA", "(30,0,30)","Proton Bragg Fit Parameter", true,"d"));


		//vars.push_back(bdt_variable("cos(atan(longest_asso_track_reco_diry/longest_asso_track_reco_dirz))","(25,0,1)","Reconstructed Track |Cosine Theta|", true,"d"));
		//vars.push_back(bdt_variable("cos(atan(longest_asso_track_reco_diry/longest_asso_track_reco_dirx))","(25,0,1)","Reconstructed Track |Cosine Phi|", true,"d"));
		vars.push_back(bdt_variable("cos(atan(reco_track_diry[longest_asso_track_index]/reco_track_dirz[longest_asso_track_index]))","(25,0,1)","Reconstructed Track |Cosine Theta|", true,"d"));
		vars.push_back(bdt_variable("cos(atan(reco_track_diry[longest_asso_track_index]/reco_track_dirx[longest_asso_track_index]))","(25,0,1)","Reconstructed Track |Cosine Phi|", true,"d"));

		//		vars.push_back(bdt_variable("cos(longest_asso_track_theta)","(25,-3.2,3.2)","Theta [Rad]", true,"d"));
		//		vars.push_back(bdt_variable("cos(longest_asso_track_phi)","(25,0,0,3.2)","Phi [Degrees]", true,"d"));
        
		vars.push_back(bdt_variable(angle_track_shower1,	"(25,0,1)","|Cosine Track-Shower Angle (Most Energetic)| ",true,"d"));
		vars.push_back(bdt_variable(angle_track_shower2,	"(25,0,1)","|Cosine Track-Shower Angle (Second Most Energetic)| ",true,"d"));
		//vars.push_back(bdt_variable("reco_asso_tracks","(5,0,4)","Number of Reconstructed Tracks",false,"i"));
        
        
	}

	std::cout<<"--------------------------------------------------------------------------"<<std::endl;
	std::cout<<"--------------------------------------------------------------------------"<<std::endl;
	for(auto &f: bdt_files){
        std::cout << f->tvertex << std::endl;
        std::cout << f->pot << std::endl;
		std::cout<<"Loading "<<f->tag<<"\t with "<<f->numberofevents<<"\t events and "<<f->tvertex->GetEntries()<<"\t verticies."<<std::endl;
		std::cout<<"POT of file loaded is: "<<f->pot<<"\t\t which is pot/event: "<<f->numberofevents/f->pot<<std::endl;
		std::cout<<"Scale factor is then: "<<f->scale_data<<std::endl;
	}
	std::cout<<"--------------------------------------------------------------------------"<<std::endl;
	std::cout<<"--------------------------------------------------------------------------"<<std::endl;



	/*
	   Best is at track_cosmicut: 0.536655 track_bnbcut: 0.500287
	   Best is at notrack.g_cosmicut: 0.525239 notrack_bnbcut: 0.508714
	   track sig: 22.1861 track bkg 511.006 0.98145
	   notrack sig: 16.8038 notrack bkg 368.859 0.874938
Combined: 1.31445 with sig 38.9899 879.865 s/sqrtb 1.31445
	 */

		//0.53842 0.505593 1.21062
	double fcoscut;
	double fbnbcut;
	if(istrack == "track"){
		fcoscut = 0.3;
		fbnbcut = 0.3;
	}else if(istrack == "notrack"){
		fcoscut = 0.3;
		fbnbcut = 0.3;
	}



	//===========================================================================================
	//===========================================================================================
	//		Main flow of the program , using OPTIONS
	//===========================================================================================
	//===========================================================================================

	if(mode_option == "train") {
		std::cout<<"**********************Starting COSMIC BDT Training*************************"<<std::endl;
		bdt_train(cosmic_bdt_info, signal_pure, bnb_pure, vars, TMVAmethods);
		std::cout<<"**********************Starting BNB BDT Training*************************"<<std::endl;
		bdt_train(bnb_bdt_info, signal_pure, bnb_pure, vars, TMVAmethods);


	}else if(mode_option == "app"){
		//Apply! This will update cosmic_bdt_info, signal file and bkg file. As in update them PROPERLY!	
		//std::vector<bdt_file*> app_files = {data5e19,bnbext,signal_pure, bnb_pure, intime, signal_cosmics, bnb_cosmics}; 
		std::vector<bdt_file*> app_files = {signal_pure, bnb_pure, signal_cosmics, bnb_cosmics}; 
		bdt_app(bnb_bdt_info, app_files, vars, TMVAmethods);
		bdt_app(cosmic_bdt_info, app_files, vars, TMVAmethods);
	}
	else if(mode_option == "response"){

		for(auto &method: TMVAmethods){
			for(int i=0; i< bdt_files.size(); i++){
				std::cout<<"Now adding TreeFriend: "<<cosmic_bdt_info.identifier<<"_app.root"<<" "<<bdt_files.at(i)->tag<<std::endl;
				bdt_files.at(i)->addFriend(bdt_files.at(i)->tag +"_"+cosmic_bdt_info.identifier,  cosmic_bdt_info.identifier+"_app"+".root");

				std::cout<<"Now adding TreeFriend: "<<bnb_bdt_info.identifier<<"_app.root"<<" "<<bdt_files.at(i)->tag<<std::endl;
				bdt_files.at(i)->addFriend(bdt_files.at(i)->tag +"_"+bnb_bdt_info.identifier,  bnb_bdt_info.identifier+"_app"+".root");
			}
		}

		//Ok print out Cosmic BDT
	//	bdt_response cosmic_response(cosmic_bdt_info, signal_pure, intime);
	//	cosmic_response.plot_bdt_response(ftest);
        
		bdt_response bnb_response(bnb_bdt_info, signal_pure, bnb_pure);
		bnb_response.plot_bdt_response(ftest);

	}	
	else if(mode_option == "recomc"){

		for(auto &method: TMVAmethods){
			for(int i=0; i< bdt_files.size(); i++){
				std::cout<<"Now adding TreeFriend: "<<cosmic_bdt_info.identifier<<"_app.root"<<" "<<bdt_files.at(i)->tag<<std::endl;
				bdt_files.at(i)->addFriend(bdt_files.at(i)->tag +"_"+cosmic_bdt_info.identifier,  cosmic_bdt_info.identifier+"_app"+".root");

				std::cout<<"Now adding TreeFriend: "<<bnb_bdt_info.identifier<<"_app.root"<<" "<<bdt_files.at(i)->tag<<std::endl;
				bdt_files.at(i)->addFriend(bdt_files.at(i)->tag +"_"+bnb_bdt_info.identifier,  bnb_bdt_info.identifier+"_app"+".root");
			}
		}

		bdt_recomc test;
		//plot_recomc(TFile *fout, bdt_file* file, bdt_variable var, double cut_cosmic_val, double cut_bnb_val){




	//test.plot_recomc(ftest, bnb_cosmics, bnb_cosmics->getBDTVariable("bnb_track") , fcoscut,fbnbcut);
	//test.plot_recomc(ftest, signal_cosmics, signal_cosmics->getBDTVariable("bnb_track") , fcoscut,fbnbcut);

		int h=0;
		for(auto &v:vars){
				test.plot_recomc(ftest, signal_cosmics, v, fcoscut, fbnbcut);
				test.plot_recomc(ftest, bnb_cosmics, v, fcoscut, fbnbcut);
				h++;
				if (h > 1) break;  
		}	
	//test.plot_recomc(ftest, bnb_cosmics, vars.at(1), fcoscut, fbnbcut);
					//test.plot_recomc(ftest, signal_cosmics, vars.at(1), fcoscut, fbnbcut);

		//		test.plot_recomc(ftest, signal_cosmics, signal_cosmics->getBDTVariable("cosmic_track"), usecut1, usecut2);
		//		test.plot_recomc(ftest, signal_cosmics, signal_cosmics->getBDTVariable("bnb_track"), usecut1, usecut2);

		//		for(int i=1; i<=4; i++){
		//			test.plot_recomc(ftest, bnb_cosmics, vars.at(i), usecut1, usecut2);
		//			test.plot_recomc(ftest, signal_cosmics, vars.at(i), usecut1, usecut2);
		//		}

	}
	else if(mode_option == "sig"){

		for(auto &method: TMVAmethods){
			for(int i=0; i< bdt_files.size(); i++){
				std::cout<<"Now adding TreeFriend: "<<cosmic_bdt_info.identifier<<"_app.root"<<" "<<bdt_files.at(i)->tag<<std::endl;
				bdt_files.at(i)->addFriend(bdt_files.at(i)->tag +"_"+cosmic_bdt_info.identifier,  cosmic_bdt_info.identifier+"_app"+".root");

				std::cout<<"Now adding TreeFriend: "<<bnb_bdt_info.identifier<<"_app.root"<<" "<<bdt_files.at(i)->tag<<std::endl;
				bdt_files.at(i)->addFriend(bdt_files.at(i)->tag +"_"+bnb_bdt_info.identifier,  bnb_bdt_info.identifier+"_app"+".root");
			}
		}


		TFile *fsig = new TFile(("significance_"+istrack+".root").c_str(),"recreate");
		std::vector<double> ans = scan_significance(fsig, {signal_cosmics} , {bnb_cosmics, intime}, cosmic_bdt_info, bnb_bdt_info);
		std::cout<<"Best Fit Significance: "<<ans.at(0)<<" "<<ans.at(1)<<" "<<ans.at(2)<<std::endl;
		fsig->Close();


	}else if(mode_option == "stack"){

		for(auto &method: TMVAmethods){
			for(int i=0; i< bdt_files.size(); i++){
				std::cout<<"Now adding TreeFriend: "<<cosmic_bdt_info.identifier<<"_app.root"<<" "<<bdt_files.at(i)->tag<<std::endl;
				bdt_files.at(i)->addFriend(bdt_files.at(i)->tag +"_"+cosmic_bdt_info.identifier,  cosmic_bdt_info.identifier+"_app"+".root");

				std::cout<<"Now adding TreeFriend: "<<bnb_bdt_info.identifier<<"_app.root"<<" "<<bdt_files.at(i)->tag<<std::endl;
				bdt_files.at(i)->addFriend(bdt_files.at(i)->tag +"_"+bnb_bdt_info.identifier,  bnb_bdt_info.identifier+"_app"+".root");
			}
		}

		bdt_stack obs("obs");
		obs.addToStack(signal_cosmics);
		obs.addToStack(bnb_cosmics);
		//obs.addToStack(intime);

		for(auto &v:vars){
			//obs.plotStacks(ftest,  v,fcoscut,fbnbcut);
		}
		obs.plotBDTStacks(ftest,bnb_bdt_info.identifier, fcoscut, fbnbcut);

	}
	else if(mode_option == "vars"){
		
        for(auto &method: TMVAmethods){
			for(int i=0; i< bdt_files.size(); i++){
				std::cout<<"Now adding TreeFriend: "<<cosmic_bdt_info.identifier<<"_app.root"<<" "<<bdt_files.at(i)->tag<<std::endl;
				bdt_files.at(i)->addFriend(bdt_files.at(i)->tag +"_"+cosmic_bdt_info.identifier,  cosmic_bdt_info.identifier+"_app"+".root");

				std::cout<<"Now adding TreeFriend: "<<bnb_bdt_info.identifier<<"_app.root"<<" "<<bdt_files.at(i)->tag<<std::endl;
				bdt_files.at(i)->addFriend(bdt_files.at(i)->tag +"_"+bnb_bdt_info.identifier,  bnb_bdt_info.identifier+"_app"+".root");
			}
		}



		for(auto &v:vars){
			TCanvas *c_var = new TCanvas(("cvar_"+v.name+"_bnb").c_str(), ("cvar_"+v.name+"_bnb").c_str(),2000,1600);
			c_var->Divide(2,2);

            // Note: changed 4 to 2, because we don't have cosmics yet
			for(int j=0; j<2;j++){	
               		        std::cout<<v.name<<" "<<j<<std::endl;
				std::string cut_signal = signal_pure->getStageCuts(j,fcoscut,fbnbcut); 
				std::string cut_bnb = bnb_pure->getStageCuts(j,fcoscut,fbnbcut); 


				TH1* sig = signal_pure->getTH1(v,cut_signal.c_str(),v.safe_name+"_sig_bnb_var" ,1.0);
				TH1* bkg = bnb_pure->getTH1(v,cut_bnb.c_str(),v.safe_name+"_bkg_bnb_var" ,1.0);
	
				sig->Scale(1.0/sig->Integral());			
				bkg->Scale(1.0/bkg->Integral());			
		//		sig->SetLineColor(kRed-7);
		//		bkg->SetLineColor(kGreen-3);
	
				c_var->cd(j+1);			
				sig->Draw("hist");
				bkg->Draw("hist same");
				//sig->GetXaxis()->SetTitle(v.unit.c_str());
				sig->GetYaxis()->SetTitle("Verticies [Area Normalized]");
				sig->GetYaxis()->SetTitleOffset(1.5);

				TLegend *l = new TLegend(0.45,0.59,0.89,0.89);
				l->SetLineColor(kWhite);
				l->SetFillStyle(0);
		
				l->AddEntry(sig,"NC #pi^{0} Signal","l");	
				l->AddEntry(bkg,"BNB Background","l");	
				l->Draw();
	
				double max_height = std::max( sig->GetMaximum(), bkg->GetMaximum());
				sig->SetMaximum(max_height*1.1);



			}
			c_var->Print(("var/bnb_"+v.safe_unit+".pdf").c_str(),"pdf");
                
		}


		if(false){
		


		for(auto &v:vars){
			TCanvas *c_var = new TCanvas(("cvar_"+v.name+"_cosmo").c_str(), ("cvar_"+v.name+"_cosmo").c_str(),2000,1600);
			c_var->Divide(2,2);


			for(int j=0; j<2;j++){	

				std::string cut_signal = signal_pure->getStageCuts(j,fcoscut,fbnbcut); 
				std::string cut_intime = intime->getStageCuts(j,fcoscut,fbnbcut); 


				TH1* sig = signal_pure->getTH1(v,cut_signal.c_str(),v.safe_name+"_sig_cosmo_var" ,1.0);
				TH1* bkg = intime->getTH1(v,cut_intime.c_str(),v.safe_name+"_bkg_cosmo_var" ,1.0);
	
				sig->Scale(1.0/sig->Integral());			
				bkg->Scale(1.0/bkg->Integral());			
				sig->SetLineColor(kRed-7);
				bkg->SetLineColor(kGreen-3);
	
				c_var->cd(j+1);			
				sig->Draw("hist");
				bkg->Draw("hist same");
				//sig->GetXaxis()->SetTitle(v.unit.c_str());
				sig->GetYaxis()->SetTitle("Verticies [Area Normalized]");
				sig->GetYaxis()->SetTitleOffset(1.5);

				TLegend *l = new TLegend(0.45,0.59,0.89,0.89);
				l->SetLineColor(kWhite);
				l->SetFillStyle(0);
		
				l->AddEntry(sig,"NC #Delta #gamma Signal","l");	
				l->AddEntry(bkg,"Intime Cosmic Background","l");	
				l->Draw();
	
				double max_height = std::max( sig->GetMaximum(), bkg->GetMaximum());
				sig->SetMaximum(max_height*1.1);



			}
			c_var->Print(("var/cosmic_"+v.safe_name+".pdf").c_str(),"pdf");
		}


		}



	}/*else if(mode_option == "eff"){

		for(auto &method: TMVAmethods){
			for(int i=0; i< bdt_files.size(); i++){
				std::cout<<"Now adding TreeFriend: "<<cosmic_bdt_info.identifier<<"_app.root"<<" "<<bdt_files.at(i)->tag<<std::endl;
				bdt_files.at(i)->addFriend(bdt_files.at(i)->tag +"_"+cosmic_bdt_info.identifier,  cosmic_bdt_info.identifier+"_app"+".root");

				std::cout<<"Now adding TreeFriend: "<<bnb_bdt_info.identifier<<"_app.root"<<" "<<bdt_files.at(i)->tag<<std::endl;
				bdt_files.at(i)->addFriend(bdt_files.at(i)->tag +"_"+bnb_bdt_info.identifier,  bnb_bdt_info.identifier+"_app"+".root");
			}
		}


		double plot_pot = 6.6e20;

		std::cout<<"Starting efficiency study: coscut @ "<<fcoscut<<" bnbcut@: "<<fbnbcut<<std::endl;

		std::cout<<"########################################################################"<<std::endl;
		std::cout<<"###############################Efficiency###############################"<<std::endl;
		for(int i=0; i< bdt_files.size(); i++){
			std::cout<<bdt_files.at(i)->tag<<std::endl;
			std::vector<double> prev;
			double start;
			double sel;		

			double pot_scale = (plot_pot/bdt_files.at(i)->pot )*bdt_files.at(i)->scale_data;
			std::string gencut = "true_nu_vtx_fid_contained == 1";
			if(bdt_files.at(i)->tag == "IntimeCosmics"){
				gencut = "1";
			}	

			double nevents = bdt_files.at(i)->tevent->GetEntries(gencut.c_str())*pot_scale;	
			double nv = bdt_files.at(i)->tvertex->GetEntries((gencut+"&&1").c_str())*pot_scale;	
			double ns = bdt_files.at(i)->tvertex->GetEntries((gencut+"&&  reco_asso_showers==1 && reco_asso_tracks "+num_track_cut).c_str())*pot_scale;	

			start = nevents;
			sel = ns;
			std::cout<<"Stage G\t\t"<<nevents<<"\t\t"<<nevents/start*100<<std::endl;
			std::cout<<"Stage V\t\t"<<nv<<"\t\t"<<nv/start*100<<std::endl;
			std::cout<<"Stage S\t\t"<<ns<<"\t\t"<<ns/start*100<<"\t\t"<<100<<std::endl;

			for(int j=0; j <4; j++){		
				std::string thiscut = bdt_files.at(i)->getStageCuts(j,fcoscut,fbnbcut); 
				double nvert =bdt_files.at(i)->tvertex->GetEntries(thiscut.c_str())*pot_scale;
				double eff =0;
				//	if(j==0){
				//		start = nvert;
				//		
				//	}

				eff = nvert/start*100;

				std::cout<<"Stage "<<std::to_string(j)<<"\t\t"<<nvert<<"\t\t"<<eff<<"\t\t"<<nvert/sel*100<<std::endl;	

			}
		}


	}
    
	else if(mode_option == "effdata"){
		std::vector<bdt_file*> data_files = {data5e19, bnbext};
		for(auto &method: TMVAmethods){
			for(int i=0; i< data_files.size(); i++){
				std::cout<<"Now adding TreeFriend: "<<cosmic_bdt_info.identifier<<"_app.root"<<" "<<data_files.at(i)->tag<<std::endl;
				data_files.at(i)->addFriend(data_files.at(i)->tag +"_"+cosmic_bdt_info.identifier,  cosmic_bdt_info.identifier+"_app"+".root");
				std::cout<<"Now adding TreeFriend: "<<bnb_bdt_info.identifier<<"_app.root"<<" "<<data_files.at(i)->tag<<std::endl;
				data_files.at(i)->addFriend(data_files.at(i)->tag +"_"+bnb_bdt_info.identifier,  bnb_bdt_info.identifier+"_app"+".root");
			}
		}

    

		std::cout<<"Starting efficiency study: coscut @ "<<fcoscut<<" bnbcut@: "<<fbnbcut<<std::endl;

		std::cout<<"########################################################################"<<std::endl;
		std::cout<<"###############################Efficiency###############################"<<std::endl;
		for(int i=0; i< data_files.size(); i++){
			std::cout<<data_files.at(i)->tag<<std::endl;
			std::vector<double> prev;
			double start;
			double sel;		

			double pot_scale = 1.0;// (plot_pot/data_files.at(i)->pot )*data_files.at(i)->scale_data;
			std::string gencut = "1";

			double nevents = data_files.at(i)->tevent->GetEntries(gencut.c_str())*pot_scale;	
			double nv = data_files.at(i)->tvertex->GetEntries((gencut+"&&1").c_str())*pot_scale;	
			double ns = data_files.at(i)->tvertex->GetEntries((gencut+"&&  reco_asso_showers==1 && reco_asso_tracks "+num_track_cut).c_str())*pot_scale;	

			start = nevents;
			sel = ns;
			std::cout<<"Stage G\t\t"<<nevents<<"\t\t"<<nevents/start*100<<std::endl;
			std::cout<<"Stage V\t\t"<<nv<<"\t\t"<<nv/start*100<<std::endl;
			std::cout<<"Stage S\t\t"<<ns<<"\t\t"<<ns/start*100<<"\t\t"<<100<<std::endl;

			for(int j=0; j <4; j++){		
				std::string thiscut = data_files.at(i)->getStageCuts(j,fcoscut,fbnbcut); 
				double nvert =data_files.at(i)->tvertex->GetEntries(thiscut.c_str())*pot_scale;
				double eff =0;
				eff = nvert/start*100;
				std::cout<<"Stage "<<std::to_string(j)<<"\t\t"<<nvert<<"\t\t"<<eff<<"\t\t"<<nvert/sel*100<<std::endl;	
				if(j==3){data_files.at(i)->tvertex->Scan( "run_number:subrun_number:event_number:reco_shower_dedx_plane2",thiscut.c_str());};
			}
		}


	}
    */
	 else if(mode_option == "precalc"){
		bdt_precalc pre(bnb_pure);
		pre.genTrackInfo();

		addPreFriends(bnb_pure,"track");
		

	
	}  else {
		std::cout << "WARNING: " << mode_option << " is an invalid option\n";
	}

	ftest->Close();

	return 0;

	}
