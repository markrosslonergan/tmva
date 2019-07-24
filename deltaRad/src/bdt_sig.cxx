#include "bdt_sig.h"



/*
How do I normalize..

Two cuts( on this single cut)

INTIME
RECO2 has 1013198 events in 31497 files
DETSIM has 1030055 events in 31980 files
I ended with 989875 events


BNBCOSMIC
DETSIM has 2412300 in 48246 files
RECO2 should have 2360950 events
I ended with 2360950 events //ooohh all of them 

Weight each intime cosmic event with a factor 10.279*N_gen_BNB/(N_gen_cosmic*my_rate)
= 10.279*2412300/(1030055*989875/1013198) = 24.639718178714663

times whatever POT scaling we need to put on the BNB events to get to 6.6e20
which for v3.0_with calo is 2.38091e+21
= 24.6397*6.6e20/2.38091e21  = 6.830246418386248

	precut--
	for loop over BDT1
	for loop over BDT2
		calc S/sqrt(S+S+BKG)

*/


std::vector<double> scan_significance( std::vector<bdt_file*> sig_files, std::vector<bdt_file*> bkg_files, std::vector<bdt_info> bdt_infos){
	std::cout<<"Starting to Scan Significance"<<std::endl;
	double best_significance = 0;
    std::vector<double> best_mva(bdt_infos.size(), DBL_MAX);
    

	double plot_pot = 13.2e20;
	

	//for nice plots make the 50, 25 is quicker tho
    int nsteps = 5;
    std::vector<double>maxvals(bdt_infos.size(),-999);

	std::cout<<"Setting stage entry lists"<<std::endl;
	for(size_t i = 0; i < sig_files.size(); ++i) {
		sig_files.at(i)->setStageEntryList(1);
	}
	for(size_t i = 0; i < bkg_files.size(); ++i) {
		bkg_files.at(i)->setStageEntryList(1);
	}

	for(size_t i = 0; i < sig_files.size(); ++i) {
        for(size_t k=0; k< bdt_infos.size(); k++){
		    double tmax_1 = sig_files.at(i)->tvertex->GetMaximum( sig_files.at(i)->getBDTVariable(bdt_infos[k]).name.c_str()    );
		    double tmax_2 = bkg_files.at(i)->tvertex->GetMaximum( sig_files.at(i)->getBDTVariable(bdt_infos[k]).name.c_str()    );
            maxvals[k] = std::max(maxvals[k], std::max(tmax_1,tmax_2));
        }
    }

    std::vector<double> minvals = maxvals;
    for(auto &v: minvals) v=v*0.5;

	//Create N2tempoary TEntryLists  at minimum
	std::vector<TEntryList*> sig_min_lists;
	std::vector<TEntryList*> bkg_min_lists;



	std::cout<<"Setting Min entry lists"<<std::endl;
	for(size_t i = 0; i < sig_files.size(); ++i) {

		std::string min_list_name  = "micam"+std::to_string(i);
		sig_files.at(i)->tvertex->Draw((">>"+min_list_name).c_str(), sig_files.at(i)->getStageCuts(1+bdt_infos.size(), minvals ).c_str() , "entrylist");
		sig_min_lists.push_back(  (TEntryList*)gDirectory->Get(min_list_name.c_str()) );
		sig_files.at(i)->tvertex->SetEntryList(sig_min_lists.back());

	}
	for(size_t i = 0; i < bkg_files.size(); ++i) {

		std::string min_list_name  = "mibam"+std::to_string(i);
		bkg_files.at(i)->tvertex->Draw((">>"+min_list_name).c_str(), bkg_files.at(i)->getStageCuts(1+bdt_infos.size(), minvals).c_str() , "entrylist");
		bkg_min_lists.push_back(  (TEntryList*)gDirectory->Get(min_list_name.c_str()) );
		bkg_files.at(i)->tvertex->SetEntryList(bkg_min_lists.back());
	}	



    std::vector<double> steps(bdt_infos.size(),0.0);
    for(int i=0; i< bdt_infos.size(); i++){
            steps[i] = (maxvals[i]-minvals[i])/((double)nsteps);
    }

    for(int i=0; i< bdt_infos.size();i++){
        std::cout<<bdt_infos[i].identifier<<" min: "<<minvals[i]<<" "<<maxvals[i]<<std::endl;
    }

    double scal_up = 1.01;
    double scal_down = 0.99;
//    std::vector<double> cval = {0.469232, 0.575191, 0.528824, 0.325369};
    double fcoscut = 0.549475;// 0.542626; //sig.1:ccut: 0.551752 0.581527  #signal: 24.1207 #bkg: 46.2715 ||  bnb: 46.2715 cos: 0 || impact 17.9759 3.54595
    double fbnbcut = 0.580694; //0.464029 0.558237


    std::vector<double> cval = {fcoscut,fbnbcut};
    if(true){
            minvals[0]=cval[0]*scal_down;
            maxvals[0]=cval[0]*scal_up;
           
            minvals[1]=cval[1]*scal_down;
            maxvals[1]=cval[1]*scal_up;
           
            minvals[2]=cval[2]*scal_down;
            maxvals[2]=cval[2]*scal_up;
           
            minvals[3]=cval[3]*scal_down;
            maxvals[3]=cval[3]*scal_up;

    }

  
  // Calculate total signal for efficiency 
  double total_sig = 0.;
 
  for(size_t i = 0; i < sig_files.size(); ++i) {
      double pot_scale = (plot_pot/sig_files.at(i)->pot )*sig_files.at(i)->scale_data;
      std::cout << "POT scale: " << pot_scale << std::endl;

      std::string bnbcut = sig_files.at(i)->getStageCuts(1,minvals); 
      total_sig += sig_files.at(i)->tvertex->GetEntries(bnbcut.c_str())*pot_scale;
  }

    TRandom3 *rangen  = new TRandom3(0);  
    std::cout<<"Starting"<<std::endl;
    for(int t=0; t < 10000; t++){

            std::vector<double> d (bdt_infos.size(),0);
            for(int i=0; i< bdt_infos.size(); i++){
                d[i] = rangen->Uniform(minvals[i], maxvals[i]);
            }
            double impact = rangen->Uniform(13,20);
//            double dist = rangen->Uniform(0,16);
            //std::string s_impact = "((sss_num_candidates==0) ||  Min$(sss_candidate_impact_parameter)>"+std::to_string(impact) +") ";
            std::string s_impact = "((sss_num_candidates==0)|| Sum$(sss_candidate_impact_parameter<"+std::to_string(impact)+ "&& sss_candidate_min_dist<70.0)==0 )";
            //std::string s_impact = "((sss_num_candidates==0)|| Min$(sss_candidate_impact_parameter/sss_candidate_min_dist)<"+std::to_string(impact)+ ")";
			
            double signal = 0;
			double background = 0;
			std::vector<double> bkg;	
		    	
            for(size_t i = 0; i < sig_files.size(); ++i) {
				double pot_scale = (plot_pot/sig_files.at(i)->pot )*sig_files.at(i)->scale_data;
			
				std::string bnbcut = sig_files.at(i)->getStageCuts(1+bdt_infos.size(), d)+"&&"+s_impact; 
				signal += sig_files.at(i)->GetEntries(bnbcut.c_str())*pot_scale;

			}

			for(size_t i = 0; i < bkg_files.size(); ++i) {
				double pot_scale = (plot_pot/bkg_files.at(i)->pot)*bkg_files.at(i)->scale_data;

                
				std::string bnbcut = bkg_files.at(i)->getStageCuts(1+bdt_infos.size(),d)+"&&"+s_impact; 
           //     std::cout<<bnbcut<<std::endl;
				bkg.push_back(bkg_files.at(i)->GetEntries(bnbcut.c_str())*pot_scale);			

				background += bkg.back();
			}


			double significance =0;
			if(signal==0){
				 significance =0;
			}else if(background !=0){
				//significance = signal/(signal+background);
				significance = signal/sqrt(background);

			}else{
				std::cout<<"method_best_significane_seperate || signal2+background2 == 0 , sig: "<<signal<<" , so significance  = nan. Woopsie."<<std::endl;
				//break;
			}

			if(significance > best_significance) {
				best_significance = significance;
				best_mva = d;
                std::cout<<"Best Sig: "<<best_significance<<std::endl;
 
		    	std::cout<<"  --ccut: IMACT: "<<impact<<" ";
                  for(int p=0; p< best_mva.size();p++){
                    std::cout<<best_mva[p]<<" dist( "<<std::min(fabs(minvals[p]-best_mva[p])/(minvals[p]+best_mva[p]),fabs(maxvals[p]-best_mva[p])/(maxvals[p]+best_mva[p]))<<")";   
               }
            }

			std::cout<<"ccut: ";
            for(auto &dd:d){
                std::cout<<dd<<" ";   
            }
            std::cout<<" #signal: "<<signal<<" #bkg: "<<background<<" || "<<" bnb: "<<bkg.at(0)<<" cos: "<<bkg.at(1)<<" || impact "<<impact<<" "<<significance<<std::endl;
		}

      std::cout<<"------------_FINAL Best Sig: "<<best_significance<<std::endl;
 
		    	std::cout<<"  --ccut: ";
                  for(auto &dd: best_mva){
                 std::cout<<dd<<" ";   
               }


	return std::vector<double>{0,0,0};

}



std::vector<double> scan_significance(TFile * fout, std::vector<bdt_file*> sig_files, std::vector<bdt_file*> bkg_files, bdt_info cosmic_focused_bdt, bdt_info bnb_focused_bdt){
	std::cout<<"Starting to Scan Significance"<<std::endl;
	double best_significance = 0;
	double best_mva_cut = DBL_MAX;
	double best_mva_cut2 = DBL_MAX;

	double plot_pot = 13.2e20;
	

	//for nice plots make the 50, 25 is quicker tho
	int nsteps_cosmic = 20;//50
	double cut_min_cosmic = 999;
	double cut_max_cosmic = -999;

	int nsteps_bnb = 20;//50
	double cut_min_bnb = 999;//0.52;
	double cut_max_bnb = -999;
	
	std::cout<<"Setting stage entry lists"<<std::endl;
	for(size_t i = 0; i < sig_files.size(); ++i) {
		sig_files.at(i)->setStageEntryList(1);
	}
	for(size_t i = 0; i < bkg_files.size(); ++i) {
		bkg_files.at(i)->setStageEntryList(1);
	}

	for(size_t i = 0; i < sig_files.size(); ++i) {
	//	double tmin_cos = sig_files.at(i)->tvertex->GetMinimum( (sig_files.at(i)->getBDTVariable(cosmic_focused_bdt).name + ">0").c_str()    );
		double tmax_cos = sig_files.at(i)->tvertex->GetMaximum( sig_files.at(i)->getBDTVariable(cosmic_focused_bdt).name.c_str()    );
		double tmax_bnb = sig_files.at(i)->tvertex->GetMaximum( sig_files.at(i)->getBDTVariable(bnb_focused_bdt).name.c_str()    );
	
		std::cout<<sig_files.at(i)->getBDTVariable(cosmic_focused_bdt).name<<" MaxCos: "<<tmax_cos<<" MaxBnb: "<<tmax_bnb<<std::endl;
		//if( tmin_cos <= cut_min_cosmic) cut_min_cosmic=tmin_cos;
		if( tmax_cos >= cut_max_cosmic) cut_max_cosmic=tmax_cos;
		if( tmax_bnb >= cut_max_bnb) cut_max_bnb=tmax_bnb;

	}
  // Normally *0.7 or *0.8
	cut_min_cosmic = cut_max_cosmic*0.8;
	cut_min_bnb = cut_max_bnb*0.8;

  // These are normally *1.0
	cut_max_cosmic =cut_max_cosmic*0.9;
	cut_max_bnb =cut_max_bnb*0.9; 

	//Zoomed in notrack
//	cut_min_cosmic = 0.54; cut_max_cosmic = 0.58;
//	cut_min_bnb = 0.51; cut_max_bnb = 0.552;

	//Best Fit Significance: 0.601552 0.533678 1.63658
	//Zoomed in track
//	cut_min_cosmic = 0.69; cut_max_cosmic = 0.71;
//	cut_min_bnb = 0.56; cut_max_bnb = 0.58;




	//Create 2 tempoary TEntryLists  at minimum
	std::vector<TEntryList*> sig_min_lists;
	std::vector<TEntryList*> bkg_min_lists;


	std::cout<<"Setting Min entry lists"<<std::endl;
	for(size_t i = 0; i < sig_files.size(); ++i) {
	

		std::string min_list_name  = "micam"+std::to_string(i);
		sig_files.at(i)->tvertex->Draw((">>"+min_list_name).c_str(), sig_files.at(i)->getStageCuts(3, cut_min_cosmic,cut_min_bnb).c_str() , "entrylist");
		sig_min_lists.push_back(  (TEntryList*)gDirectory->Get(min_list_name.c_str()) );
		sig_files.at(i)->tvertex->SetEntryList(sig_min_lists.back());


	}
	for(size_t i = 0; i < bkg_files.size(); ++i) {

		std::string min_list_name  = "mibam"+std::to_string(i);
		bkg_files.at(i)->tvertex->Draw((">>"+min_list_name).c_str(), bkg_files.at(i)->getStageCuts(3, cut_min_cosmic,cut_min_bnb).c_str() , "entrylist");
		bkg_min_lists.push_back(  (TEntryList*)gDirectory->Get(min_list_name.c_str()) );
		bkg_files.at(i)->tvertex->SetEntryList(bkg_min_lists.back());


	}	




	std::cout<<"BNB sig scan from: "<<cut_min_bnb<<" to "<<cut_max_bnb<<std::endl;
	std::cout<<"COSMIC sig scan from: "<<cut_min_cosmic<<" to "<<cut_max_cosmic<<std::endl;

	double step_cosmic = (cut_max_cosmic-cut_min_cosmic)/((double)nsteps_cosmic);
	double step_bnb = (cut_max_bnb-cut_min_bnb)/((double)nsteps_bnb);


	TH2D * h2_sig_cut = new TH2D( "significance_2D",  "significance_2D",nsteps_cosmic, cut_min_cosmic, cut_max_cosmic, nsteps_bnb, cut_min_bnb, cut_max_bnb);
	std::vector<double> vec_sig;//some vectors to store TGraph info;
	std::vector<double> vec_cut;	
  
  // Calculate total signal for efficiency 
  double total_sig = 0.;
  for(size_t i = 0; i < sig_files.size(); ++i) {
      double pot_scale = (plot_pot/sig_files.at(i)->pot )*sig_files.at(i)->scale_data;
      std::cout << "POT scale: " << pot_scale << std::endl;

      std::string bnbcut = sig_files.at(i)->getStageCuts(1,-9,-9); 
      total_sig += sig_files.at(i)->tvertex->GetEntries(bnbcut.c_str())*pot_scale;
  }

	for(int di=1; di<=nsteps_cosmic; di++) {
		double d  = (double)(di-1.0)*step_cosmic + cut_min_cosmic; ;	

		for(int di2=1; di2<=nsteps_bnb; di2++) {
			double d2  = (double)(di2-1.0)*step_bnb + cut_min_bnb ;	

			double signal = 0;
			double background = 0;
			std::vector<double> bkg;	

			for(size_t i = 0; i < sig_files.size(); ++i) {
				double pot_scale = (plot_pot/sig_files.at(i)->pot )*sig_files.at(i)->scale_data;
			
				std::string bnbcut = sig_files.at(i)->getStageCuts(3,d,d2); 
				signal += sig_files.at(i)->GetEntries(bnbcut.c_str())*pot_scale;

			}

			for(size_t i = 0; i < bkg_files.size(); ++i) {
				double pot_scale = (plot_pot/bkg_files.at(i)->pot)*bkg_files.at(i)->scale_data;
		
	
				std::string bnbcut = bkg_files.at(i)->getStageCuts(3,d,d2); 
				bkg.push_back(	bkg_files.at(i)->GetEntries(bnbcut.c_str())*pot_scale);			

				background += bkg.back();
			}



			double significance =0;
			if(signal==0){
				 significance =0;
			}else if(background !=0){
                //significance = signal/(signal+background)*signal/total_sig*100;
				significance = signal/sqrt(background);
			}else{
				std::cout<<"method_best_significane_seperate || signal2+background2 == 0, so significance  = nan @ cut1: "<<d<<", cut2: "<<d2<<std::endl;
				break;
			}


			if(significance > best_significance) {
				best_significance = significance;
				best_mva_cut = d;
				best_mva_cut2 = d2;
			}


			std::cout<<"ccut: "<<d<<" bcut: "<<d2<<" "<<" #signal: "<<signal<<" #bkg: "<<background<<" || "<<" bnb: "<<bkg.at(0)<<" cos: "<<bkg.at(1)<<" || "<<significance<<std::endl;
			vec_sig.push_back(significance);
			vec_cut.push_back(d2);
			h2_sig_cut->SetBinContent(di,di2, significance);
		}

	}
		

	h2_sig_cut->SetStats(false);
	TCanvas * c_sig_cuts =  new TCanvas( "significance_cuts_colz", "significance_cuts_colz", 2000,1600 );
	c_sig_cuts->Divide(2,1);
	TPad *p1 = (TPad*)c_sig_cuts->cd(1);
	p1->SetRightMargin(0.13);
	h2_sig_cut->Draw("colz");
	h2_sig_cut->GetXaxis()->SetTitle("Cosmic Cut");
	h2_sig_cut->GetYaxis()->SetTitle("BNB Cut");
	
   	std::vector<double> vec_bf_cut1 = {best_mva_cut};
   	std::vector<double> vec_bf_cut2 = {best_mva_cut2};
	TGraph *graph_bf = new TGraph(vec_bf_cut1.size(), &vec_bf_cut1[0], &vec_bf_cut2[0]);
	graph_bf->SetMarkerStyle(29);
	graph_bf->SetMarkerSize(2);
	graph_bf->SetMarkerColor(kBlack);
	graph_bf->Draw("same p");
 

	TGraph * graph_cut = new TGraph(vec_sig.size(), &vec_cut[0], &vec_sig[0]);
	graph_cut->SetTitle("1D slices");
	c_sig_cuts->cd(2);
	graph_cut->Draw("alp");

	h2_sig_cut->Write();
	c_sig_cuts->Write();

	return std::vector<double>{best_mva_cut, best_mva_cut2, best_significance};

}



std::vector<double> lin_scan(std::vector<bdt_file*> sig_files, std::vector<bdt_file*> bkg_files, bdt_info cosmic_focused_bdt, bdt_info bnb_focused_bdt, double c1, double c2){
	std::cout<<"Starting to Scan Significance"<<std::endl;
	double best_significance = 0;
	double best_mva_cut = DBL_MAX;
	double best_mva_cut2 = DBL_MAX;

//	double plot_pot = 6.6e20;
	double plot_pot = 13.2e20;
	

	std::vector<double> vec_sig;//some vectors to store TGraph info;
	std::vector<double> vec_cut;	


	double d1 = c1;
	double d2 = c2; 


	for(int i=0; i< 100; i++){

			d1 = d1*1.0001;
			//d1 = d1*0.99999;

			double signal = 0;
			double background = 0;
			std::vector<double> bkg;	

			for(size_t i = 0; i < sig_files.size(); ++i) {
				double pot_scale = (plot_pot/sig_files.at(i)->pot )*sig_files.at(i)->scale_data;
			
				std::string bnbcut = sig_files.at(i)->getStageCuts(3,d1,d2); 
				signal += sig_files.at(i)->GetEntries(bnbcut.c_str())*pot_scale;

			}

			for(size_t i = 0; i < bkg_files.size(); ++i) {
				double pot_scale = (plot_pot/bkg_files.at(i)->pot)*bkg_files.at(i)->scale_data;
		
	
				std::string bnbcut = bkg_files.at(i)->getStageCuts(3,d1,d2); 
				bkg.push_back(	bkg_files.at(i)->GetEntries(bnbcut.c_str())*pot_scale);			

				background += bkg.back();
			}
			double significance =0;
			if(signal==0){
				 significance =0;
			}else if(background !=0){
				significance = signal/sqrt(background);
			}else{
				std::cout<<"method_best_significane_seperate || signal2+background2 == 0, so significance  = nan @ cut1: "<<d1<<", cut2: "<<d2<<std::endl;
				break;
			}


			if(significance > best_significance) {
				best_significance = significance;
				best_mva_cut = d1;
				best_mva_cut2 = d2;
			}


			std::cout<<"ccut: "<<d1<<" bcut: "<<d2<<" "<<" #signal: "<<signal<<" #bkg: "<<background<<" || "<<" bnb: "<<bkg.at(0)<<" cos: "<<bkg.at(1)<<" || "<<significance<<std::endl;
			vec_sig.push_back(significance);
			vec_cut.push_back(d1);
			vec_cut.push_back(d2);
		}

		
	return std::vector<double>{best_mva_cut, best_mva_cut2, best_significance};

}

