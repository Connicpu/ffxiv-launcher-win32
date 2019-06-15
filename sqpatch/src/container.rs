use crate::{Parse, Position, Stream};

use byteorder::{ReadBytesExt, BE};

#[derive(Copy, Clone)]
pub struct Container {
    /// Does not include the 4 bytes at the beginning of the data
    pub length: u32,
    pub signature: [u8; 4],
    pub data: Position,
}

impl Container {
    pub fn read_contents<D>(&self, stream: &mut impl Stream) -> std::io::Result<D>
    where
        D: Parse,
    {
        stream.seek_to(self.data)?;
        D::parse(stream)
    }
}

impl Parse for Container {
    fn parse(stream: &mut impl Stream) -> std::io::Result<Self> {
        let length = stream.read_u32::<BE>()?;
        let mut signature = [0; 4];
        stream.read_exact(&mut signature)?;
        let data = stream.pos()?;
        stream.seek_by(length as i64 + 4)?;

        Ok(Container {
            length,
            signature,
            data,
        })
    }
}

impl std::fmt::Debug for Container {
    fn fmt(&self, fmt: &mut std::fmt::Formatter) -> std::fmt::Result {
        let sig = String::from_utf8_lossy(&self.signature);
        fmt.debug_struct("Container")
            .field("length", &self.length)
            .field("signature", &sig)
            .field("data", &self.data)
            .finish()
    }
}
