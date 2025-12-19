function data_water_content(input_temp_file, input_sun_file, output_file)
    % data_water_content
    %   Build a daily meteorological table (temperature, precipitation, radiation)
    %   for the ST QUENTIN station and write it to a CSV file.
    %
    % Parameters
    %   input_temp_file : path to temperature / precipitation file
    %   input_sun_file  : path to solar radiation file
    %   output_file     : path to output CSV file
    
        %% 1) Temperature and precipitation data
    
        temp_table = readtable(input_temp_file);
        station_name = 'ST QUENTIN';
    
        % Filter selected station
        mask_station_temp = strcmp(temp_table.NOM_USUEL, station_name);
        selected_vars_temp = {'AAAAMMJJ', 'RR', 'TNTXM'};
        daily_temp_precip = temp_table(mask_station_temp, selected_vars_temp);
    
        % Daily datetime and month identifier (YYYYMM)
        daily_temp_precip.DATE_dt = datetime(daily_temp_precip.AAAAMMJJ, ...
                                             'ConvertFrom', 'yyyymmdd');
        daily_temp_precip.MonthID = year(daily_temp_precip.DATE_dt) * 100 + ...
                                    month(daily_temp_precip.DATE_dt);
    
        % Monthly mean of TNTXM (one row per month)
        Tmean = groupsummary(daily_temp_precip, "MonthID", "mean", "TNTXM");
        Tmean.Properties.VariableNames{'mean_TNTXM'} = 'TNTXM_monthly_mean';
        Tmean = Tmean(:, {'MonthID', 'TNTXM_monthly_mean'});
    
        % Add monthly temperature mean back to daily table
        daily_temp_precip = outerjoin(daily_temp_precip, Tmean, ...
            "Keys", "MonthID", "MergeKeys", true, "Type", "left");
    
        %% 2) Solar radiation data
    
        sun_table = readtable(input_sun_file);
        mask_station_sun = strcmp(sun_table.NOM_USUEL, station_name);
        selected_vars_sun = {'AAAAMMJJ', 'GLOT'};
        daily_radiation = sun_table(mask_station_sun, selected_vars_sun);
    
        daily_radiation.DATE_dt = datetime(daily_radiation.AAAAMMJJ, ...
                                           'ConvertFrom', 'yyyymmdd');
        daily_radiation.MonthID = year(daily_radiation.DATE_dt) * 100 + ...
                                  month(daily_radiation.DATE_dt);
    
        % Monthly mean of GLOT (one row per month)
        Pmean = groupsummary(daily_radiation, "MonthID", "mean", "GLOT");
        Pmean.Properties.VariableNames{'mean_GLOT'} = 'GLOT_monthly_mean';
        Pmean = Pmean(:, {'MonthID', 'GLOT_monthly_mean'});
    
        % Add monthly radiation mean to daily temperature/precipitation table
        daily_temp_precip = outerjoin(daily_temp_precip, Pmean, ...
            "Keys", "MonthID", "MergeKeys", true, "Type", "left");
    
        %% 3) Time window filtering
    
        % Remove helper variables
        daily_temp_precip = removevars(daily_temp_precip, {'DATE_dt', 'MonthID'});
    
        % Time range for the study (can be adjusted if needed)
        start_date = 19890101;
        end_date   = 20701231;
    
        mask_time = (daily_temp_precip.AAAAMMJJ >= start_date) & ...
                    (daily_temp_precip.AAAAMMJJ <= end_date);
    
        filtered_temp_precip = daily_temp_precip(mask_time, :);
    
        % Write final table
        writetable(filtered_temp_precip, output_file);
    end
    