function data_water_content(input_temp_file, input_sun_file, output_file)

    % données météorologiques
    
    T = readtable(input_temp_file); 
    St_Quentin = 'ST QUENTIN';
    
    % Filtrer la station
    mask_St_quentin = strcmp(T.NOM_USUEL, St_Quentin);
    vars = {'AAAAMMJJ','RR','TNTXM'};
    Table_temp_precip = T(mask_St_quentin, vars);
    
    % Date journalière et identifiant de mois
    Table_temp_precip.DATE_dt = datetime(Table_temp_precip.AAAAMMJJ, ...
                                         'ConvertFrom','yyyymmdd');
    Table_temp_precip.MonthID = year(Table_temp_precip.DATE_dt)*100 + ...
                                month(Table_temp_precip.DATE_dt);
    
    % Moyenne mensuelle de TNTXM (une ligne par mois)
    Tmean = groupsummary(Table_temp_precip, "MonthID", "mean", "TNTXM");
    Tmean.Properties.VariableNames{'mean_TNTXM'} = 'TNTXM_moy_mois';
    Tmean = Tmean(:, {'MonthID','TNTXM_moy_mois'});   % seulement clé + moyenne
    
    % Ajouter la moyenne mensuelle à la table journalière
    Table_temp_precip = outerjoin(Table_temp_precip, Tmean, ...
        "Keys","MonthID", "MergeKeys", true, "Type","left");
    
    % rayonnement 
    P = readtable(input_sun_file);
    mask_St_quentin_2 = strcmp(P.NOM_USUEL, St_Quentin);
    variables = {'AAAAMMJJ', 'GLOT'};
    Table_radiation = P(mask_St_quentin_2, variables);
    
    Table_radiation.DATE_dt = datetime(Table_radiation.AAAAMMJJ, ...
                                       'ConvertFrom','yyyymmdd');
    Table_radiation.MonthID = year(Table_radiation.DATE_dt)*100 + ...
                              month(Table_radiation.DATE_dt);
    
    % Moyenne mensuelle de GLOT (une ligne par mois)
    Pmean = groupsummary(Table_radiation, "MonthID", "mean", "GLOT");
    Pmean.Properties.VariableNames{'mean_GLOT'} = 'GLOT_moy_mois';
    Pmean = Pmean(:, {'MonthID','GLOT_moy_mois'});    % seulement clé + moyenne
    
    % Ajouter la moyenne mensuelle de GLOT à la table journalière
    Table_temp_precip = outerjoin(Table_temp_precip, Pmean, ...
        "Keys","MonthID", "MergeKeys", true, "Type","left");
    
    
    Table_temp_precip = removevars(Table_temp_precip, {'DATE_dt','MonthID'});
    % bornes de temps de notre étude, à modifier si besoins ( je ne suis pas
    % sure des dates ) 
    
    date_debut = 19890101;
    date_fin   = 20701231;
    
    mask = (Table_temp_precip.AAAAMMJJ >= date_debut) & ...
           (Table_temp_precip.AAAAMMJJ <= date_fin);
    
    Table_temp_precip_filtre = Table_temp_precip(mask, :);
    
    writetable( Table_temp_precip_filtre, output_file); 
    
    end