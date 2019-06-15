use crate::{Parse, Stream};

use std::io;

use byteorder::{ReadBytesExt, BE};

pub const SQPK_SIG: u32 = 0x53_51_50_4B; // SQPK

#[derive(Copy, Clone, Debug)]
pub struct Aply {
    pub data: [u32; 4],
}

impl Parse for Aply {
    fn parse(stream: &mut impl Stream) -> io::Result<Aply> {
        let data = [
            stream.read_u32::<BE>()?,
            stream.read_u32::<BE>()?,
            stream.read_u32::<BE>()?,
            stream.read_u32::<BE>()?,
        ];

        Ok(Aply { data })
    }
}
