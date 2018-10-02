//========================================================================
// 'vga_module.v' adapted from:
//
// https://github.com/milanvidakovic/FPGAComputer/blob/master/vga_module.v
//========================================================================

module top( // top module and signals wired to FPGA pins
	clk25,
	rr,
	rg,
	rb,
	hs,
	vs,

	wclk,
	d0,
	dc,
	cs
);

chars chars_1( // Character generator
  .char(curr_char[7:0]),
  .rownum(y[2:0]),
  .pixels(pixels)
);

input clk25; // Oscillator input 25MHz
output rr; // VGA Red 1-bit
output rg; // VGA Green 1-bit
output rb; // VGA Blue 1-bit
output hs; // H-sync pulse
output vs; // V-sync pulse

input wclk;
input d0;
input dc;
input cs;

reg r, g, b;
reg [9:0] x;
reg [9:0] y;
wire valid;
wire [7:0] curr_char;
wire [7:0] pixels; // Pixels making up one row of the character

assign hs = x < (640 + 16) || x >= (640 + 16 + 96);
assign vs = y < (480 + 10) || y >= (480 + 10 + 2);
assign rr = r;
assign rg = g;
assign rb = b;
assign valid = (x < 640) && (y < 480);
assign curr_char = dout;

parameter addr_width = 13;
parameter data_width = 8;
reg  [data_width-1:0] mem [(1<<addr_width)-1:0];
reg  [data_width-1:0] dout;
wire [addr_width-1:0] raddr;
reg  [addr_width-1:0] raddr_r = 0;
assign raddr = raddr_r;
reg  [data_width-1:0] din;
wire [addr_width-1:0] waddr;
reg  [addr_width-1:0] waddr_r = 0;
assign waddr = waddr_r;
reg [2:0] bit_r = 0;

always @(posedge wclk) // Write memory
begin
	if (cs) begin
		if (dc) begin
			din[bit_r] <= d0;
			bit_r <= bit_r + 1; // Increment bit
		end
		else begin // Latch data
			bit_r <= 0;
			mem[waddr] <= din;
			waddr_r <= waddr_r + 1; // Increment address
		end
	end
	else begin
		waddr_r <= 0; // 'VSYNC'
	end
end

always @(negedge clk25)
begin
	dout <= mem[raddr]; // Read memory
end

always @(posedge clk25)
begin
	if (x < 10'd799) begin
		x <= x + 1'b1;
	end
	else begin
		x <= 10'b0;
		if (y < 10'd524) begin
			y <= y + 1'b1;
		end
		else begin
			y <= 10'b0;
		end
	end

	if (x >= 640) begin
		if ((x >= 640) && (y >= 480)) begin
			// when we start the vertical blanking, we need to fetch in advance the first character (0, 0)
			raddr_r <= 0;
		end
		else if ((x >= 640) && ((y & 7) < 7)) begin
			// when we start the horizontal blanking, and still displaying character in the current line,
			// we need to fetch in advance the first character in the current line (0, y)
			raddr_r <= ((y >> 3)*80);
		end
		else if ((x >= 640) && ((y & 7) == 7)) begin
			// when we start the horizontal blanking, and we need to go to the next line,
			// we need to fetch in advance the first character in next line (0, y+1)
			raddr_r <= (((y >> 3) + 1)*80);
		end
	end // if (!valid)
	// from this moment on, x and y are valid
	else if (x < 640) begin
		if ((x & 7) == 7) begin
			// when we are finishing current character, we need to fetch in advance the next character (x+1, y)
			// at the last pixel of the current character, let's fetch next
			raddr_r <= ((x >> 3) + (y >> 3)*80 + 1);
		end
	end

	if (valid) begin
		r <= pixels[7 - (x & 7)]; // ? !curr_char[6+8] : curr_char[2+8];
		g <= pixels[7 - (x & 7)]; // ? !curr_char[5+8] : curr_char[1+8];
		b <= pixels[7 - (x & 7)]; // ? !curr_char[4+8] : curr_char[0+8];
	end
	else begin
		// blanking -> no pixels
		r <= 1'b0;
		g <= 1'b0;
		b <= 1'b0;
	end
end

initial begin
	x <= 10'b0;
	y <= 10'b0;
end

endmodule