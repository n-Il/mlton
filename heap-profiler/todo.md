arrays and stacks arent getting their source locations input into the header
    see rssatossa sequenceAlloc for example

should be able to swap storing the sourceIndex with the sourceNameIndex by modifying the implement profiling

going through and making sure there's no unnecesarry code running outside of our usecase

modifications to split the 32 bit header for both sourceNameIndex and garbage collections survived

modification towards forcing on the location profiling when compiled with heap profiling(would need to make sure the file output is also still being specified) 

ensure compatibility with other runtime and compilation features/options 

I don't think the legend in the graphing code for the lifetimes is accurate to the bounds for when an accuracy is different than 1, I believe some edge values are included
