function plot_poro(input_file_V, input_file_P, input_file_T, output_png_file)
    % plot_poro
    %   Create porosity and pressure time series plots from input CSV files.
    %
    % Parameters
    %   input_file_V    : path to verification porosity file (V)
    %   input_file_P    : path to bulk density file part 2 (P)
    %   input_file_T    : path to bulk density file part 1 (T)
    %   output_png_file : path to output PNG file
    
        %% Read input tables
        V = readtable(input_file_V);      % verification / reference table
        P = readtable(input_file_P);      % bulk density table (part 2)
        T = readtable(input_file_T);      % bulk density table (part 1)
    
        % Convert numeric dates (YYYYMMDD) to datetime
        V.Date_interv = datetime(V.Date_interv, 'ConvertFrom', 'yyyymmdd');
    
        % Round lower depth limits in P to the next multiple of 5 cm
        P.lower_limit_int = ceil(P.lower_limit_int / 5) * 5;
    
        %% Build combined bulk density table (T followed by P)
        T_sub = T(:, {'Plot', 'DateOfMeasurement', 'LowerLimit_cm_', 'Poro_mean'});
        T_sub.Properties.VariableNames = {'Plot', 'DateOfMeasurement', 'LowerLimit_cm', 'Porosity'};
    
        P_sub = P(:, {'Plot', 'DateOfMeasurement', 'lower_limit_int', 'Poro'});
        P_sub.Properties.VariableNames = {'Plot', 'DateOfMeasurement', 'LowerLimit_cm', 'Porosity'};
    
        bulk_table = [T_sub; P_sub];
    
        %% Select horizon 1 (5 cm) for plot 1 from bulk_table
        depth_h1_cm   = 5;
        target_plot   = 1;
    
        mask_depth_h1 = (bulk_table.LowerLimit_cm == depth_h1_cm);
        bulk_h1       = bulk_table(mask_depth_h1, :);
    
        mask_plot_h1  = (bulk_h1.Plot == target_plot);
        bulk_h1_plot  = bulk_h1(mask_plot_h1, :);
    
        dates_bulk_h1 = bulk_h1_plot.DateOfMeasurement;
        poro_bulk_h1  = bulk_h1_plot.Porosity;
    
        %% Select corresponding records in V for horizon index 1
        horizon_index_h1 = 1;
    
        mask_V_h1      = (V.Horizon == horizon_index_h1);
        V_h1           = V(mask_V_h1, :);
        dates_V_h1     = V_h1.Date_interv;
        poro_V_h1      = V_h1.Porosity;
    
        %% Select horizon 5 (25 cm) for plot 1 from bulk_table
        depth_h5_cm    = 25;
    
        mask_depth_h5  = (bulk_table.LowerLimit_cm == depth_h5_cm);
        bulk_h5        = bulk_table(mask_depth_h5, :);
    
        mask_plot_h5   = (bulk_h5.Plot == target_plot);
        bulk_h5_plot   = bulk_h5(mask_plot_h5, :);
    
        dates_bulk_h5  = bulk_h5_plot.DateOfMeasurement;
        poro_bulk_h5   = bulk_h5_plot.Porosity;
    
        %% Select corresponding records in V for horizon index 5
        horizon_index_h5 = 5;
    
        mask_V_h5      = (V.Horizon == horizon_index_h5);
        V_h5           = V(mask_V_h5, :);
        dates_V_h5     = V_h5.Date_interv;
        poro_V_h5      = V_h5.Porosity;
    
        %% Compute daily sum of maximum pressure
        pressure_daily = groupsummary(V, "Date_interv", "day", "sum", "PressionMax_kPa");
        % pressure_daily.day_Date_interv      : unique dates
        % pressure_daily.sum_PressionMax_kPa : daily sum of pressures
    
        %% Create figure with three subplots
        fig = figure('Visible', 'on');
    
        % -------- Horizon 1 --------
        subplot(3, 1, 1);
        plot(dates_bulk_h1, poro_bulk_h1, '-o', 'DisplayName', 'BD (T+P) - H1');
        hold on;
        plot(dates_V_h1,    poro_V_h1,    '-s', 'DisplayName', 'Verification - H1');
        hold off;
        xlabel('Date');
        ylabel('Porosity');
        title('Measured and Modeled Porosity Over Time – Horizon 1');
        legend('Location', 'best');
        grid on;
    
        % -------- Horizon 5 --------
        subplot(3, 1, 2);
        plot(dates_bulk_h5, poro_bulk_h5, '-o', 'DisplayName', 'BD (T+P) - H5');
        hold on;
        plot(dates_V_h5,    poro_V_h5,    '-s', 'DisplayName', 'Verification - H5');
        hold off;
        xlabel('Date');
        ylabel('Porosity');
        title('Measured and Modeled Porosity Over Time – Horizon 5');
        legend('Location', 'best');
        grid on;
    
        % -------- Sum of pressures --------
        subplot(3, 1, 3);
        plot(dates_V_h5, pressure_daily.sum_PressionMax_kPa, '-o', 'LineWidth', 1.5);
        grid on;
        xlabel('Date');
        ylabel('Sum of Pressures (kPa)');
        title('Total Applied Soil Pressure Over Time');
    
        % Save figure to file
        drawnow;
        exportgraphics(fig, output_png_file, 'Resolution', 300);
    end
    