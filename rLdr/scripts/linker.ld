SECTIONS
{
    .text :
    {
        *( .text$A )
        *( .text$B )
        *( .rdata* )
        *( .text$F )
    }
}
