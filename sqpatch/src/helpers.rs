use std::fmt::UpperHex;

pub struct HexFmt<'a, T: UpperHex>(pub &'a T);

impl<T: UpperHex> std::fmt::Debug for HexFmt<'_, T> {
    fn fmt(&self, fmt: &mut std::fmt::Formatter) -> std::fmt::Result {
        self.0.fmt(fmt)
    }
}
