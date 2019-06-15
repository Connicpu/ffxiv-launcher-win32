use crate::{Parse, Position, Stream};

use byteorder::{ReadBytesExt, BE};

#[derive(Copy, Clone, Debug)]
pub struct Sqpk {
    pub length: u32,
    pub data: Position,
    pub kind: PackType,
}

impl Sqpk {
    pub fn read_contents<D>(&self, stream: &mut impl Stream) -> std::io::Result<D>
    where
        D: Parse,
    {
        stream.seek_to(self.data)?;
        D::parse(stream)
    }
}

impl Parse for Sqpk {
    fn parse(stream: &mut impl Stream) -> std::io::Result<Sqpk> {
        let length = stream.read_u32::<BE>()?;
        let data = stream.pos()?;
        let kind = PackType(stream.read_u16::<BE>()?);
        stream.seek_by(length as i64 - 2)?;

        Ok(Sqpk { length, data, kind })
    }
}

#[derive(Copy, Clone, PartialEq, Eq)]
pub struct PackType(pub u16);

impl PackType {
    pub const T: PackType = PackType(0x5400);
    pub const X: PackType = PackType(0x5800);
    pub const FA: PackType = PackType(0x4641);
}

impl std::fmt::Debug for PackType {
    fn fmt(&self, fmt: &mut std::fmt::Formatter) -> std::fmt::Result {
        if self.0 & 0xFF != 0 {
            write!(
                fmt,
                "\"{}{}\"",
                (self.0 >> 8) as u8 as char,
                self.0 as u8 as char
            )
        } else {
            write!(fmt, "\"{}\"", (self.0 >> 8) as u8 as char)
        }
    }
}
