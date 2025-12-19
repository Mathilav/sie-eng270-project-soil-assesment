function extraction_porosity(input_file_agricultural_work, input_file_ITK, input_file_theta, output_file)
    % extraction_porosity
    %   Build a daily table of operations, pressure and water content
    %   for plot 9 and write it to a CSV file.
    %
    % Parameters
    %   input_file_agricultural_work : path to agricultural work file (operations)
    %   input_file_ITK               : path to ITK file (implements / pressure)
    %   input_file_theta             : path to theta output file (water content)
    %   output_file                  : path to output CSV file
    
        %% 1) Operations table for plot 9
    
        ops_table = readtable(input_file_agricultural_work);
        itk_table = readtable(input_file_ITK);
    
        % Keep only plot 9
        ops_table = ops_table(ops_table.Plot == 9, :);
    
        % Keep useful variables and join with ITK to get pressure
        ops_table = ops_table(:, {'Crop', 'Date_interv', 'Passage', 'ITK'});
        ops_table.ITK = categorical(ops_table.ITK);
        itk_table.ITK = categorical(itk_table.ITK);
    
        ops_table = innerjoin(ops_table, itk_table, 'Keys', 'ITK');
        ops_table = ops_table(:, {'Date_interv', 'Passage', 'PressionMax_kPa_'});
    
        % Convert dates from dd/MM/yyyy text to numeric yyyymmdd
        dt = datetime(ops_table.Date_interv, "InputFormat", "dd/MM/yyyy");
        ops_table.Date_interv = yyyymmdd(dt);
    
        % Sort by date
        ops_table = sortrows(ops_table, 'Date_interv');
    
        %% 2) Add missing days (no operations)
    
        dt_all        = datetime(string(ops_table.Date_interv), "InputFormat", "yyyyMMdd");
        all_dates_num = yyyymmdd((min(dt_all) : max(dt_all))');
    
        % Identify missing days
        missing_dates = setdiff(all_dates_num, ops_table.Date_interv);
        n_missing     = numel(missing_dates);
    
        missing_table = table;
        missing_table.Date_interv      = missing_dates;
        missing_table.Passage          = zeros(n_missing, 1);
        missing_table.PressionMax_kPa_ = zeros(n_missing, 1);
    
        full_ops_table = [ops_table; missing_table];
        full_ops_table = sortrows(full_ops_table, 'Date_interv');
    
        %% 3) Add water content from theta_output.csv
    
        theta_table = readtable(input_file_theta);
        % Columns: Date_Theta, theta
        theta_table.Properties.VariableNames = {'Date_interv', 'WaterContent'};
    
        % Ensure Date_interv is numeric yyyymmdd
        if ~isnumeric(theta_table.Date_interv)
            theta_table.Date_interv = yyyymmdd( ...
                datetime(theta_table.Date_interv, "InputFormat", "yyyyMMdd"));
        end
    
        % Left outer join: keep all dates from full_ops_table, add water content when available
        full_ops_table = outerjoin(full_ops_table, theta_table, ...
            "Keys", "Date_interv", ...
            "Type", "left", ...
            "MergeKeys", true);
    
        % Replace missing WaterContent values with 0
        full_ops_table.WaterContent(ismissing(full_ops_table.WaterContent)) = 0;
    
        % Write final table to file
        writetable(full_ops_table, output_file);
    end
    